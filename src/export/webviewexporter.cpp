#include "webviewexporter.h"

#include <QWidget>
#include <QWebEnginePage>
#include <QFileInfo>

#include <widgets/editors/markdownviewer.h>
#include <widgets/editors/editormarkdownvieweradapter.h>
#include <core/editorconfig.h>
#include <core/markdowneditorconfig.h>
#include <core/configmgr.h>
#include <core/htmltemplatehelper.h>
#include <utils/utils.h>
#include <utils/pathutils.h>
#include <utils/fileutils.h>
#include <utils/webutils.h>
#include <core/file.h>

using namespace vnotex;

static const QString c_imgRegExp = "<img ([^>]*)src=\"([^\"]+)\"([^>]*)>";

WebViewExporter::WebViewExporter(QWidget *p_parent)
    : QObject(p_parent)
{
}

void WebViewExporter::clear()
{
    m_askedToStop = false;

    delete m_viewer;
    m_viewer = nullptr;

    m_htmlTemplate.clear();
    m_exportHtmlTemplate.clear();

    m_exportOngoing = false;
}

bool WebViewExporter::doExport(const ExportOption &p_option,
                               const File *p_file,
                               const QString &p_outputFile)
{
    bool ret = false;
    m_askedToStop = false;

    Q_ASSERT(p_file->getContentType().isMarkdown());

    Q_ASSERT(!m_exportOngoing);
    m_exportOngoing = true;

    m_webViewStates = WebViewState::Started;

    auto baseUrl = PathUtils::pathToUrl(p_file->getContentPath());
    m_viewer->setHtml(m_htmlTemplate, baseUrl);
    m_viewer->adapter()->setText(p_file->read());

    while (!isWebViewReady()) {
        Utils::sleepWait(100);

        if (m_askedToStop) {
            goto exit_export;
        }

        if (isWebViewFailed()) {
            qWarning() << "WebView failed when exporting" << p_file->getFilePath();
            goto exit_export;
        }
    }

    qDebug() << "WebView is ready";

    // Add extra wait to make sure Web side is really ready.
    Utils::sleepWait(200);

    switch (p_option.m_targetFormat) {
    case ExportFormat::HTML:
        Q_ASSERT(p_option.m_htmlOption);
        // TODO: not supported yet.
        Q_ASSERT(!p_option.m_htmlOption->m_useMimeHtmlFormat);
        ret = doExportHtml(*p_option.m_htmlOption, p_outputFile, baseUrl);
        break;

    default:
        break;
    }

exit_export:
    m_exportOngoing = false;
    return ret;
}

void WebViewExporter::stop()
{
    m_askedToStop = true;
}

bool WebViewExporter::isWebViewReady() const
{
    return m_webViewStates == (WebViewState::LoadFinished | WebViewState::WorkFinished);
}

bool WebViewExporter::isWebViewFailed() const
{
    return m_webViewStates & WebViewState::Failed;
}

bool WebViewExporter::doExportHtml(const ExportHtmlOption &p_htmlOption,
                                   const QString &p_outputFile,
                                   const QUrl &p_baseUrl)
{
    // 0 - Busy
    // 1 - Finished
    // -1 - Failed
    int state = 0;

    connect(m_viewer->adapter(), &MarkdownViewerAdapter::contentReady,
            this, [&, this](const QString &p_headContent, const QString &p_styleContent, const QString &p_bodyContent) {
                qDebug() << "doExportHtml contentReady";
                // Maybe unnecessary. Just to avoid duplicated signal connections.
                disconnect(m_viewer->adapter(), &MarkdownViewerAdapter::contentReady, this, 0);

                if (p_bodyContent.isEmpty() || m_askedToStop) {
                    state = -1;
                    return;
                }

                if (!writeHtmlFile(p_outputFile,
                                   p_baseUrl,
                                   p_headContent,
                                   p_styleContent,
                                   p_bodyContent,
                                   p_htmlOption.m_embedStyles,
                                   p_htmlOption.m_completePage,
                                   p_htmlOption.m_embedImages)) {
                    state = -1;
                    return;
                }

                state = 1;
            });

    m_viewer->adapter()->saveContent();


    while (state == 0) {
        Utils::sleepWait(100);

        if (m_askedToStop) {
            break;
        }
    }

    return state == 1;
}

bool WebViewExporter::writeHtmlFile(const QString &p_file,
                                    const QUrl &p_baseUrl,
                                    const QString &p_headContent,
                                    QString p_styleContent,
                                    const QString &p_bodyContent,
                                    bool p_embedStyles,
                                    bool p_completePage,
                                    bool p_embedImages)
{
    const auto baseName = QFileInfo(p_file).completeBaseName();
    auto title = QString("%1 - %2").arg(baseName, ConfigMgr::c_appName);
    const QString resourceFolderName = baseName + "_files";
    auto resourceFolder = PathUtils::concatenateFilePath(PathUtils::parentDirPath(p_file), resourceFolderName);

    qDebug() << "HTML files folder" << resourceFolder;

    auto htmlTemplate = m_exportHtmlTemplate;
    HtmlTemplateHelper::fillTitle(htmlTemplate, title);

    if (!p_styleContent.isEmpty() && p_embedStyles) {
        embedStyleResources(p_styleContent);
        HtmlTemplateHelper::fillStyleContent(htmlTemplate, p_styleContent);
    }

    if (!p_headContent.isEmpty()) {
        HtmlTemplateHelper::fillHeadContent(htmlTemplate, p_headContent);
    }


    if (p_completePage) {
        QString bodyContent(p_bodyContent);
        if (p_embedImages) {
            embedBodyResources(p_baseUrl, bodyContent);
        } else {
            fixBodyResources(p_baseUrl, resourceFolder, bodyContent);
        }

        HtmlTemplateHelper::fillBodyContent(htmlTemplate, bodyContent);
    } else {
        HtmlTemplateHelper::fillBodyContent(htmlTemplate, p_bodyContent);
    }

    FileUtils::writeFile(p_file, htmlTemplate);

    // Delete empty resource folder.
    QDir dir(resourceFolder);
    if (dir.exists() && dir.isEmpty()) {
        dir.cdUp();
        dir.rmdir(resourceFolderName);
    }

    return true;
}

void WebViewExporter::prepare(const ExportOption &p_option)
{
    Q_ASSERT(!m_viewer && !m_exportOngoing);

    {
        // Adapter will be managed by MarkdownViewer.
        auto adapter = new MarkdownViewerAdapter(this);
        m_viewer = new MarkdownViewer(adapter, QColor(), 1, static_cast<QWidget *>(parent()));
        m_viewer->hide();
        connect(m_viewer->page(), &QWebEnginePage::loadFinished,
                this, [this]() {
                    m_webViewStates |= WebViewState::LoadFinished;
                });
        connect(adapter, &MarkdownViewerAdapter::workFinished,
                this, [this]() {
                    m_webViewStates |= WebViewState::WorkFinished;
                });
    }

    const auto &config = ConfigMgr::getInst().getEditorConfig().getMarkdownEditorConfig();
    m_htmlTemplate = HtmlTemplateHelper::generateMarkdownViewerTemplate(config,
                                                                        p_option.m_renderingStyleFile,
                                                                        p_option.m_syntaxHighlightStyleFile,
                                                                        p_option.m_useTransparentBg);

    bool addOutlinePanel = false;
    if (p_option.m_htmlOption) {
        addOutlinePanel = p_option.m_htmlOption->m_addOutlinePanel;
    }
    m_exportHtmlTemplate = HtmlTemplateHelper::generateExportTemplate(config, addOutlinePanel);
}

bool WebViewExporter::embedStyleResources(QString &p_html) const
{
    bool altered = false;
    QRegExp reg("\\burl\\(\"((file|qrc):[^\"\\)]+)\"\\);");

    int pos = 0;
    while (pos < p_html.size()) {
        int idx = p_html.indexOf(reg, pos);
        if (idx == -1) {
            break;
        }

        QString dataURI = WebUtils::toDataUri(QUrl(reg.cap(1)), false);
        if (dataURI.isEmpty()) {
            pos = idx + reg.matchedLength();
        } else {
            // Replace the url string in html.
            QString newUrl = QString("url('%1');").arg(dataURI);
            p_html.replace(idx, reg.matchedLength(), newUrl);
            pos = idx + newUrl.size();
            altered = true;
        }
    }

    return altered;
}

bool WebViewExporter::embedBodyResources(const QUrl &p_baseUrl, QString &p_html)
{
    bool altered = false;
    if (p_baseUrl.isEmpty()) {
        return altered;
    }

    QRegExp reg(c_imgRegExp);

    int pos = 0;
    while (pos < p_html.size()) {
        int idx = p_html.indexOf(reg, pos);
        if (idx == -1) {
            break;
        }

        if (reg.cap(2).isEmpty()) {
            pos = idx + reg.matchedLength();
            continue;
        }

        QUrl srcUrl(p_baseUrl.resolved(reg.cap(2)));
        const auto dataURI = WebUtils::toDataUri(srcUrl, true);
        if (dataURI.isEmpty()) {
            pos = idx + reg.matchedLength();
        } else {
            // Replace the url string in html.
            QString newUrl = QString("<img %1src='%2'%3>").arg(reg.cap(1), dataURI, reg.cap(3));
            p_html.replace(idx, reg.matchedLength(), newUrl);
            pos = idx + newUrl.size();
            altered = true;
        }
    }

    return altered;
}

static QString getResourceRelativePath(const QString &p_file)
{
    int idx = p_file.lastIndexOf('/');
    int idx2 = p_file.lastIndexOf('/', idx - 1);
    Q_ASSERT(idx > 0 && idx2 < idx);
    return "." + p_file.mid(idx2);
}

bool WebViewExporter::fixBodyResources(const QUrl &p_baseUrl,
                                       const QString &p_folder,
                                       QString &p_html)
{
    bool altered = false;
    if (p_baseUrl.isEmpty()) {
        return altered;
    }

    QRegExp reg(c_imgRegExp);

    int pos = 0;
    while (pos < p_html.size()) {
        int idx = p_html.indexOf(reg, pos);
        if (idx == -1) {
            break;
        }

        if (reg.cap(2).isEmpty()) {
            pos = idx + reg.matchedLength();
            continue;
        }

        QUrl srcUrl(p_baseUrl.resolved(reg.cap(2)));
        QString targetFile = WebUtils::copyResource(srcUrl, p_folder);
        if (targetFile.isEmpty()) {
            pos = idx + reg.matchedLength();
        } else {
            // Replace the url string in html.
            QString newUrl = QString("<img %1src=\"%2\"%3>").arg(reg.cap(1), getResourceRelativePath(targetFile), reg.cap(3));
            p_html.replace(idx, reg.matchedLength(), newUrl);
            pos = idx + newUrl.size();
            altered = true;
        }
    }

    return altered;
}
