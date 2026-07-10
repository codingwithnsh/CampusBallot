#pragma once

#include <QObject>
#include <QVariantMap>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QString>
#include <optional>
#include <mutex>
#include "src/core/models/ElectionConfiguration.h" // Dependency on ElectionConfiguration model
#include "src/core/models/FieldDefinition.h"      // Dependency on FieldDefinition model
// #include "src/core/models/ThemeConfig.h"          // Dependency on ThemeConfig model
#include "src/core/Models.h"                      // Dependency on Core::Candidate and Core::Party

namespace Ballot::Core {

/**
 * @brief The ConfigurationManager class is a singleton responsible for managing a wide array
 * of application configurations and data.
 *
 * This class currently handles:
 * - Persistence (saving, loading, deleting) for Election, Field Definition, Theme, Party, and Candidate configurations.
 * - Providing default configurations for various election aspects (student identification, rules, results).
 * - Import/Export functionalities for configurations and themes.
 * - Configuration migration.
 * - Basic validation for election configurations and field definitions.
 *
 * @note This class currently violates the Single Responsibility Principle (SRP) by aggregating
 * too many distinct responsibilities. This leads to high coupling, reduced testability, and
 * makes the class complex to understand and maintain.
 *
 * @warning Future refactoring should aim to decompose this class into smaller, more focused
 * managers or services, each responsible for a single aspect (e.g., ElectionConfigService,
 * ThemeService, PartyService, CandidateService, FieldDefinitionService, DefaultConfigProvider,
 * ConfigPersistenceService).
 */
class ConfigurationManager : public QObject {
    Q_OBJECT
public:
    /**
     * @brief Returns the singleton instance of the ConfigurationManager.
     * @return Reference to the ConfigurationManager instance.
     */
    static ConfigurationManager& instance();
    
    /**
     * @brief Initializes the ConfigurationManager, ensuring necessary directories exist.
     * @return True if initialization is successful, false otherwise.
     */
    bool initialize();

    /**
     * @brief Shuts down the ConfigurationManager (currently no specific shutdown logic).
     */
    void shutdown();
    
    // --- Election Configuration Management ---
    /**
     * @brief Saves an election configuration to persistent storage.
     * @param config The ElectionConfiguration object to save.
     * @return True if successful, false otherwise.
     */
    bool saveElectionConfiguration(const Models::ElectionConfiguration& config);

    /**
     * @brief Loads an election configuration by its ID.
     * @param electionId The ID of the election configuration to load.
     * @return An optional containing the ElectionConfiguration if found, std::nullopt otherwise.
     */
    std::optional<Models::ElectionConfiguration> loadElectionConfiguration(const QString& electionId);

    /**
     * @brief Loads all available election configurations.
     * @return A list of all ElectionConfiguration objects.
     */
    QList<Models::ElectionConfiguration> loadAllElectionConfigurations();

    /**
     * @brief Deletes an election configuration by its ID.
     * @param electionId The ID of the election configuration to delete.
     * @return True if successful, false otherwise.
     */
    bool deleteElectionConfiguration(const QString& electionId);
    
    // --- Field Definition Management ---
    /**
     * @brief Saves a field definition.
     * @param field The FieldDefinition object to save.
     * @return True if successful, false otherwise.
     */
    bool saveFieldDefinition(const Models::FieldDefinition& field);

    /**
     * @brief Loads a field definition by its ID.
     * @param fieldId The ID of the field definition to load.
     * @return An optional containing the FieldDefinition if found, std::nullopt otherwise.
     */
    std::optional<Models::FieldDefinition> loadFieldDefinition(const QString& fieldId);

    /**
     * @brief Loads all available field definitions.
     * @return A list of all FieldDefinition objects.
     */
    Models::FieldDefinitionList loadAllFieldDefinitions();

    /**
     * @brief Deletes a field definition by its ID.
     * @param fieldId The ID of the field definition to delete.
     * @return True if successful, false otherwise.
     */
    bool deleteFieldDefinition(const QString& fieldId);
    
    // --- Theme Configuration Management ---
    /**
     * @brief Saves a theme configuration.
     * @param theme The ThemeConfig object to save.
     * @return True if successful, false otherwise.
     */
    bool saveThemeConfig(const Models::ThemeConfig& theme);

    /**
     * @brief Loads a theme configuration by its ID.
     * @param themeId The ID of the theme configuration to load.
     * @return An optional containing the ThemeConfig if found, std::nullopt otherwise.
     */
    std::optional<Models::ThemeConfig> loadThemeConfig(const QString& themeId);

    /**
     * @brief Loads all available theme configurations.
     * @return A list of all ThemeConfig objects.
     */
    QList<Models::ThemeConfig> loadAllThemeConfigs();

    /**
     * @brief Deletes a theme configuration by its ID.
     * @param themeId The ID of the theme configuration to delete.
     * @return True if successful, false otherwise.
     */
    bool deleteThemeConfig(const QString& themeId);
    
    // --- Party Management (using Core::Party) ---
    /**
     * @brief Saves a party.
     * @param party The Core::Party object to save.
     * @return True if successful, false otherwise.
     */
    bool saveParty(const Models::Party& party);

    /**
     * @brief Loads a party by its ID.
     * @param partyId The ID of the party to load.
     * @return An optional containing the Core::Party if found, std::nullopt otherwise.
     */
    std::optional<Models::Party> loadParty(const QString& partyId);

    /**
     * @brief Loads parties associated with a specific election.
     * @param electionId The ID of the election.
     * @return A list of Core::Party objects.
     */
    QList<Models::Party> loadParties(const QString& electionId);

    /**
     * @brief Deletes a party by its ID.
     * @param partyId The ID of the party to delete.
     * @return True if successful, false otherwise.
     */
    bool deleteParty(const QString& partyId);
    
    // --- Candidate Management (using Core::Candidate) ---
    /**
     * @brief Saves a candidate.
     * @param candidate The Core::Candidate object to save.
     * @return True if successful, false otherwise.
     */
    bool saveCandidate(const Models::Candidate& candidate);

    /**
     * @brief Loads a candidate by its ID.
     * @param candidateId The ID of the candidate to load.
     * @return An optional containing the Core::Candidate if found, std::nullopt otherwise.
     */
    std::optional<Models::Candidate> loadCandidate(const QString& candidateId);

    /**
     * @brief Loads candidates associated with a specific election.
     * @param electionId The ID of the election.
     * @return A list of Core::Candidate objects.
     */
    QList<Models::Candidate> loadCandidates(const QString& electionId);

    /**
     * @brief Deletes a candidate by its ID.
     * @param candidateId The ID of the candidate to delete.
     * @return True if successful, false otherwise.
     */
    bool deleteCandidate(const QString& candidateId);
    
    // --- Default Configuration Providers ---
    /**
     * @brief Provides a default ElectionConfiguration based on a specified type.
     * @param type The type of election (e.g., StudentCouncil, Custom).
     * @return A default ElectionConfiguration object.
     */
    Models::ElectionConfiguration getDefaultElectionConfig(Models::ElectionType type);

    /**
     * @brief Provides a default ThemeConfig.
     * @return A default ThemeConfig object.
     */
    Models::ThemeConfig getDefaultThemeConfig();

    /**
     * @brief Provides a default StudentIdentificationConfig.
     * @return A default StudentIdentificationConfig object.
     */
    Models::StudentIdentificationConfig getDefaultStudentIdentificationConfig();

    /**
     * @brief Provides a default StudentFieldConfig.
     * @return A default StudentFieldConfig object.
     */
    Models::StudentFieldConfig getDefaultStudentFieldConfig();

    /**
     * @brief Provides a default CandidateRulesConfig.
     * @return A default CandidateRulesConfig object.
     */
    Models::CandidateRulesConfig getDefaultCandidateRulesConfig();

    /**
     * @brief Provides a default VotingRulesConfig.
     * @return A default VotingRulesConfig object.
     */
    Models::VotingRulesConfig getDefaultVotingRulesConfig();

    /**
     * @brief Provides a default ResultSettingsConfig.
     * @return A default ResultSettingsConfig object.
     */
    Models::ResultSettingsConfig getDefaultResultSettingsConfig();

    /**
     * @brief Provides a list of default student field definitions.
     * @return A list of default FieldDefinition objects for students.
     */
    Models::FieldDefinitionList getDefaultStudentFields();

    /**
     * @brief Provides a list of default candidate field definitions.
     * @return A list of default FieldDefinition objects for candidates.
     */
    Models::FieldDefinitionList getDefaultCandidateFields();
    
    // --- Import/Export Functionality ---
    /**
     * @brief Exports the entire application configuration to a specified file.
     * @param filePath The path to the file where configuration will be exported.
     * @return True if successful, false otherwise.
     */
    bool exportConfiguration(const QString& filePath);

    /**
     * @brief Imports application configuration from a specified file.
     * @param filePath The path to the file from which configuration will be imported.
     * @return True if successful, false otherwise.
     */
    bool importConfiguration(const QString& filePath);

    /**
     * @brief Exports a specific theme configuration to a file.
     * @param theme The ThemeConfig object to export.
     * @param filePath The path to the file where the theme will be exported.
     * @return True if successful, false otherwise.
     */
    bool exportTheme(const Models::ThemeConfig& theme, const QString& filePath);

    /**
     * @brief Imports a theme configuration from a file.
     * @param filePath The path to the file from which the theme will be imported.
     * @param theme Reference to a ThemeConfig object to populate with imported data.
     * @return True if successful, false otherwise.
     */
    bool importTheme(const QString& filePath, Models::ThemeConfig& theme);
    
    // --- Configuration Migration ---
    /**
     * @brief Migrates configurations from an older version to a newer one.
     * @param fromVersion The version number to migrate from.
     * @param toVersion The version number to migrate to.
     * @return True if migration is successful, false otherwise.
     */
    bool migrateConfiguration(int fromVersion, int toVersion);
    
    // --- Configuration Validation ---
    /**
     * @brief Validates an election configuration.
     * @param config The ElectionConfiguration to validate.
     * @return A list of error messages; empty if valid.
     */
    QList<QString> validateElectionConfiguration(const Models::ElectionConfiguration& config);

    /**
     * @brief Validates a field definition.
     * @param field The FieldDefinition to validate.
     * @return A list of error messages; empty if valid.
     */
    QList<QString> validateFieldDefinition(const Models::FieldDefinition& field);
    
signals:
    /**
     * @brief Emitted when any configuration managed by this class changes.
     * @param key A string identifying the type of configuration that changed.
     */
    void configurationChanged(const QString& key);

    /**
     * @brief Emitted when an election configuration is saved.
     * @param electionId The ID of the saved election configuration.
     */
    void electionConfigurationSaved(const QString& electionId);

    /**
     * @brief Emitted when a theme configuration is saved.
     * @param themeId The ID of the saved theme configuration.
     */
    void themeConfigSaved(const QString& themeId);

    /**
     * @brief Emitted when a party is saved.
     * @param partyId The ID of the saved party.
     */
    void partySaved(const QString& partyId);

    /**
     * @brief Emitted when a candidate is saved.
     * @param candidateId The ID of the saved candidate.
     */
    void candidateSaved(const QString& candidateId);

    /**
     * @brief Emitted when a field definition is saved.
     * @param fieldId The ID of the saved field definition.
     */
    void fieldDefinitionSaved(const QString& fieldId);
    
private:
    /**
     * @brief Private constructor to enforce singleton pattern.
     */
    ConfigurationManager();

    /**
     * @brief Private destructor.
     */
    ~ConfigurationManager();
    
    /**
     * @brief Gets the base directory for storing configuration files.
     * @return The absolute path to the configuration directory.
     */
    QString getConfigDirectory() const;

    /**
     * @brief Gets the file path for a specific election configuration.
     * @param electionId The ID of the election.
     * @return The absolute file path.
     */
    QString getElectionConfigPath(const QString& electionId) const;

    /**
     * @brief Gets the file path for a specific field definition.
     * @param fieldId The ID of the field.
     * @return The absolute file path.
     */
    QString getFieldDefinitionPath(const QString& fieldId) const;

    /**
     * @brief Gets the file path for a specific theme configuration.
     * @param themeId The ID of the theme.
     * @return The absolute file path.
     */
    QString getThemeConfigPath(const QString& themeId) const;

    /**
     * @brief Gets the file path for a specific party.
     * @param partyId The ID of the party.
     * @return The absolute file path.
     */
    QString getPartyPath(const QString& partyId) const;

    /**
     * @brief Gets the file path for a specific candidate.
     * @param candidateId The ID of the candidate.
     * @return The absolute file path.
     */
    QString getCandidatePath(const QString& candidateId) const;
    
    /**
     * @brief Ensures that all necessary configuration directories exist.
     * @return True if directories exist or were created successfully, false otherwise.
     */
    bool ensureDirectories();

    /**
     * @brief Creates default configuration files if they don't already exist.
     */
    void createDefaultConfigurations();
    
    mutable std::mutex m_mutex; ///< Mutex for thread-safe access to shared resources.
    bool m_initialized = false; ///< Flag indicating if the manager is initialized.
    QString m_configDir; ///< The base directory where configurations are stored.

    // Private copy constructor and assignment operator to prevent copying
    ConfigurationManager(const ConfigurationManager&) = delete;
    ConfigurationManager& operator=(const ConfigurationManager&) = delete;
};

} // namespace Ballot::Core