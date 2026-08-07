cmake_minimum_required(VERSION 3.20)
project(TheRealCampusBallot VERSION 1.0.0 LANGUAGES CXX)

# Disable moc_predefs.h generation to avoid AutoMoc subprocess errors on some systems
set(CMAKE_CXX_COMPILER_PREDEFINES_COMMAND "")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Sql Network Charts Svg PrintSupport)

qt_add_executable(TheRealCampusBallot
    main.cpp
    # Core
    src/core/SystemManager.cpp
    src/core/ThemeManager.cpp
    src/core/config/KioskConfiguration.cpp
    src/core/config/ApplicationBootstrap.cpp
    # Security
    src/modules/security/AES256Provider.cpp
    src/modules/security/HashProvider.cpp
    src/modules/security/DigitalSignature.cpp
    src/modules/security/TamperDetector.cpp
    # Storage
    src/modules/storage/providers/SQLiteStorageProvider.cpp
    src/modules/storage/TestAdmissionStorage.cpp
    # Auth
    src/modules/auth/AuthManager.cpp
    src/modules/auth/RBACManager.cpp
    # Business modules
    src/modules/audit/AuditManager.cpp
    src/modules/backup/BackupManager.cpp
    src/modules/election/ElectionManager.cpp
    src/modules/election/VoteManager.cpp
    src/modules/plugin/PluginManager.cpp
    src/modules/integration/FirebaseRealtimeSyncManager.cpp
    # UI components
    src/ui/components/AcrylicWidget.cpp
    src/ui/components/AnimatedButton.cpp
    src/ui/components/StatCard.cpp
    src/ui/components/ToastNotification.cpp
    src/ui/components/Sidebar.cpp
    # View models
    src/ui/viewmodels/DashboardViewModel.cpp
    src/ui/viewmodels/AuthViewModel.cpp
    src/ui/viewmodels/ResultsViewModel.cpp
    # Views
    src/ui/views/MainWindow.cpp
    src/ui/views/SplashScreen.cpp
    src/ui/views/SetupWizard.cpp
    src/ui/views/LoginView.cpp
    src/ui/views/DashboardView.cpp
    src/ui/views/VotingKiosk.cpp
    src/ui/views/ResultsView.cpp
    src/ui/views/AdminPanel.cpp
    src/ui/views/UserManagementView.cpp
    src/ui/views/SettingsView.cpp
    # Dialogs
    src/ui/dialogs/CandidateFormDialog.cpp
    resources.qrc
)

target_include_directories(TheRealCampusBallot PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_features(TheRealCampusBallot PRIVATE cxx_std_20)

if(MSVC)
    target_compile_options(TheRealCampusBallot PRIVATE /W4 /permissive-)
else()
    target_compile_options(TheRealCampusBallot PRIVATE -Wall -Wextra -Wpedantic)
endif()

target_link_libraries(TheRealCampusBallot PRIVATE 
    Qt6::Core 
    Qt6::Gui 
    Qt6::Widgets 
    Qt6::Sql 
    Qt6::Network
    Qt6::Charts
    Qt6::Svg
    Qt6::PrintSupport
)

include(GNUInstallDirs)

install(TARGETS TheRealCampusBallot
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(WIN32)
    set_target_properties(TheRealCampusBallot PROPERTIES
        WIN32_EXECUTABLE ON
    )

    if(QT_VERSION_MAJOR EQUAL 6)
        qt_generate_deploy_app_script(
            TARGET TheRealCampusBallot
            OUTPUT_SCRIPT qt_deploy_script
            NO_UNSUPPORTED_PLATFORM_ERROR
        )

        install(SCRIPT ${qt_deploy_script})
    endif()
endif()
