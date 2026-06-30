cmake_minimum_required(VERSION 3.20)
project(TheRealCampusBallot VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Sql Network Charts Svg PrintSupport)

set(SOURCES
    main.cpp
    src/core/SystemManager.cpp
    src/security/AES256Provider.cpp
    src/security/HashProvider.cpp
    src/security/DigitalSignature.cpp
    src/security/TamperDetector.cpp
    src/modules/storage/providers/SQLiteStorageProvider.cpp
    src/modules/storage/TestAdmissionStorage.cpp
    src/modules/auth/AuthManager.cpp
    src/modules/auth/RBACManager.cpp
    src/modules/audit/AuditManager.cpp
    src/modules/backup/BackupManager.cpp
    src/modules/election/ElectionManager.cpp
    src/modules/election/VoteManager.cpp
    src/modules/plugin/PluginManager.cpp
    src/ui/components/AcrylicWidget.cpp
    src/ui/components/AnimatedButton.cpp
    src/ui/components/StatCard.cpp
    src/ui/components/ToastNotification.cpp
    src/ui/components/Sidebar.cpp
    src/ui/viewmodels/DashboardViewModel.cpp
    src/ui/viewmodels/AuthViewModel.cpp
    src/ui/viewmodels/ResultsViewModel.cpp
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
    src/ui/dialogs/CandidateFormDialog.cpp
)

set(HEADERS
    src/core/Models.h
    src/core/SystemManager.h
    src/core/Utils.h
    src/core/Constants.h
    src/security/IEncryptionProvider.h
    src/security/AES256Provider.h
    src/security/HashProvider.h
    src/security/DigitalSignature.h
    src/security/TamperDetector.h
    src/modules/storage/interfaces/IStorageProvider.h
    src/modules/storage/providers/SQLiteStorageProvider.h
    src/modules/storage/TestAdmissionStorage.h
    src/modules/auth/IAuthProvider.h
    src/modules/auth/AuthManager.h
    src/modules/auth/RBACManager.h
    src/modules/audit/AuditManager.h
    src/modules/backup/BackupManager.h
    src/modules/election/ElectionManager.h
    src/modules/election/VoteManager.h
    src/modules/plugin/IPlugin.h
    src/modules/plugin/PluginManager.h
    src/ui/components/AcrylicWidget.h
    src/ui/components/AnimatedButton.h
    src/ui/components/StatCard.h
    src/ui/components/ToastNotification.h
    src/ui/components/Sidebar.h
    src/ui/viewmodels/DashboardViewModel.h
    src/ui/viewmodels/AuthViewModel.h
    src/ui/viewmodels/ResultsViewModel.h
    src/ui/views/MainWindow.h
    src/ui/views/SplashScreen.h
    src/ui/views/SetupWizard.h
    src/ui/views/LoginView.h
    src/ui/views/DashboardView.h
    src/ui/views/VotingKiosk.h
    src/ui/views/ResultsView.h
    src/ui/views/AdminPanel.h
    src/ui/views/UserManagementView.h
    src/ui/views/SettingsView.h
    src/ui/dialogs/CandidateFormDialog.h
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(RESOURCES
    resources.qrc
)

add_executable(TheRealCampusBallot ${SOURCES} ${HEADERS} ${RESOURCES})

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

if(WIN32)
    set_target_properties(TheRealCampusBallot PROPERTIES
        WIN32_EXECUTABLE ON
    )

    get_target_property(_qt_core_dll Qt6::Core LOCATION)
    get_filename_component(_qt_bin_dir "${_qt_core_dll}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")

    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET TheRealCampusBallot POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}"
                    --compiler-runtime
                    "$<TARGET_FILE:TheRealCampusBallot>"
            COMMENT "Deploying Qt and MinGW dependencies..."
        )
    endif()
endif()
