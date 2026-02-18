add_subdirectory(Organizers)

target_sources(Gorgon
    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "${PROJECT_SOURCE_DIR}/Source"
    FILES
        ../UI.h
        Component.h
        ComponentStack.h
        ComponentStackWidget.h
        ScrollingWidget.h
        Dialog.h
        Dimension.h
        Helpers.h
        LayerAdapter.h
        RadioControl.h
        Template.h
        TwoStateControl.h
        TooltipManager.h
        Validators.h
        Widget.h
        WidgetContainer.h
        Window.h
)

target_sources(Gorgon
    PRIVATE
    FILES
        UI.cpp
        ComponentStack.cpp
        ScrollingWidget.cpp
        Dialog.cpp
        LayerAdapter.cpp
        Template.cpp
        TooltipManager.cpp
        Widget.cpp
        WidgetContainer.cpp
        Window.cpp
)
