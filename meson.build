project('topwindow', 'cpp', version: '0.1.0')

gtk4_dep = dependency('gtk4', version: '>=4.0.0')
libxml2_dep = dependency('libxml-2.0')
cairo_dep = dependency('cairo')

executable('topwindow',
    'src/main.cpp',
    'src/core/AdbManager.cpp',
    'src/core/CacheManager.cpp',
    'src/core/XmlParser.cpp',
    'src/models/UiNode.cpp',
    'src/models/UiTreeModel.cpp',
    'src/ui/MainWindow.cpp',
    'src/ui/HierarchyWindow.cpp',
    'src/ui/NodeAttributesPanel.cpp',
    dependencies: [gtk4_dep, libxml2_dep, cairo_dep],
    install: true
)

# Install desktop file
install_data('data/applications/com.example.TopWindow.desktop',
    install_dir: get_option('datadir') / 'applications'
)

# Install icons
icon_sizes = ['256x256']
foreach size : icon_sizes
    install_data('data/icons/hicolor/@0@/apps/com.example.TopWindow.png'.format(size),
        install_dir: get_option('datadir') / 'icons' / 'hicolor' / size / 'apps'
    )
endforeach

# Also install a fallback icon
install_data('data/icons/com.example.TopWindow.png',
    install_dir: get_option('datadir') / 'pixmaps'
)

