#include <coreplugin/locator/commandlocator.h>
    addAutoReleasedObject(new VcsSubmitEditorFactory(&submitParameters,
        []() { return new GitSubmitEditor(&submitParameters); }));
    Context submitContext(Constants::GITSUBMITEDITOR_ID);
    const int lineNumber = VcsBaseEditor::lineNumberOfCurrentEditor(state.currentFile());
    VcsBaseEditorWidget::testDiffFileResolving(editorParameters[3].id);

    VcsBaseEditorWidget::testLogResolving(editorParameters[1].id, data,