#ifndef HTMLTEMPLATEHELPER_H
#define HTMLTEMPLATEHELPER_H

#include <QString>

namespace vnotex
{
    class MarkdownEditorConfig;

    // Global options to be passed to Web side at the very beginning.
    struct WebGlobalOptions
    {
        bool m_webPlantUml = true;

        bool m_webGraphviz = true;

        bool m_sectionNumberEnabled = true;

        int m_sectionNumberBaseLevel = 2;

        bool m_constrainImageWidthEnabled = true;

        bool m_protectFromXss = false;

        bool m_htmlTagEnabled = true;

        bool m_autoBreakEnabled = false;

        bool m_linkifyEnabled = true;

        bool m_indentFirstLineEnabled = false;

        QString toJavascriptObject() const;
    };

    // Help to generate and update HTML templates.
    class HtmlTemplateHelper
    {
    public:
        HtmlTemplateHelper() = delete;

        static const QString &getMarkdownViewerTemplate();
        static void updateMarkdownViewerTemplate(const MarkdownEditorConfig &p_config);

        static QString generateMarkdownViewerTemplate(const MarkdownEditorConfig &p_config,
                                                      const QString &p_webStyleSheetFile,
                                                      const QString &p_highlightStyleSheetFile,
                                                      bool p_useTransparentBg);

        static QString generateExportTemplate(const MarkdownEditorConfig &p_config,
                                              bool p_addOutlinePanel);

        static void fillTitle(QString &p_template, const QString &p_title);

        static void fillStyleContent(QString &p_template, const QString &p_styles);

        static void fillHeadContent(QString &p_template, const QString &p_head);

        static void fillBodyContent(QString &p_template, const QString &p_body);

    private:
        struct Template
        {
            int m_revision = -1;
            QString m_template;
        };

        // Template for MarkdownViewer.
        static Template s_markdownViewerTemplate;
    };
}

#endif // HTMLTEMPLATEHELPER_H
