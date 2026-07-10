#pragma once

#include <QString>
#include <QVariant>
#include <QJsonObject>
#include <QJsonArray>
#include <QMetaType>
#include <optional>
#include <variant>
#include <vector>

namespace Ballot::Core::Models {

enum class FieldType {
    Text,
    LongText,
    Number,
    Integer,
    Decimal,
    Boolean,
    Date,
    DateTime,
    Time,
    Email,
    Phone,
    URL,
    Image,
    File,
    SelectSingle,
    SelectMultiple,
    RadioGroup,
    CheckboxGroup,
    AutoComplete,
    Slider,
    Rating,
    ColorPicker,
    Signature,
    Barcode,
    QRCode,
    RFID,
    Biometric,
    Location,
    Currency,
    Percentage,
    Custom
};

enum class FieldValidation {
    None,
    Required,
    MinLength,
    MaxLength,
    MinValue,
    MaxValue,
    Pattern,
    Email,
    Phone,
    URL,
    Unique,
    CustomRegex,
    CustomFunction
};

enum class FieldVisibility {
    Always,
    AdminOnly,
    CandidateOnly,
    VoterOnly,
    Conditional
};

struct FieldConstraint {
    FieldValidation type = FieldValidation::None;
    QVariant value;
    QString errorMessage;
    
    QJsonObject toJson() const;
    static FieldConstraint fromJson(const QJsonObject& obj);
};

struct FieldOption {
    QString value;
    QString label;
    QString description;
    QVariant metadata;
    bool isDefault = false;
    int sortOrder = 0;
    
    QJsonObject toJson() const;
    static FieldOption fromJson(const QJsonObject& obj);
};

struct FieldConditional {
    QString dependsOnField;
    QString operatorType; // "equals", "notEquals", "contains", "notContains", "in", "notIn", "gt", "lt", "gte", "lte"
    QVariant value;
    FieldVisibility visibility = FieldVisibility::Always;
    
    QJsonObject toJson() const;
    static FieldConditional fromJson(const QJsonObject& obj);
};

struct FieldDefinition {
    QString id;
    QString name;
    QString label;
    QString placeholder;
    QString helpText;
    QString groupName;
    QString category;
    
    FieldType type = FieldType::Text;
    QVariant defaultValue;
    bool isRequired = false;
    bool isUnique = false;
    bool isIndexed = false;
    bool isSearchable = true;
    bool isSortable = true;
    bool isExportable = true;
    bool isEditable = true;
    bool isVisible = true;
    bool readOnly = false;
    
    int minLength = 0;
    int maxLength = 0;
    double minValue = 0;
    double maxValue = 0;
    int step = 1;
    QString pattern;
    QString customValidation;
    
    QList<FieldOption> options;
    QList<FieldConstraint> constraints;
    QList<FieldConditional> conditionals;
    
    int sortOrder = 0;
    QString cssClass;
    QString icon;
    QVariantMap metadata;
    
    QJsonObject toJson() const;
    static FieldDefinition fromJson(const QJsonObject& obj);
    
    static FieldDefinition createTextField(const QString& id, const QString& label, bool required = false);
    static FieldDefinition createNumberField(const QString& id, const QString& label, bool required = false);
    static FieldDefinition createSelectField(const QString& id, const QString& label, const QList<FieldOption>& options, bool required = false);
    static FieldDefinition createDateField(const QString& id, const QString& label, bool required = false);
    static FieldDefinition createImageField(const QString& id, const QString& label, bool required = false);
    static FieldDefinition createEmailField(const QString& id, const QString& label, bool required = false);
    static FieldDefinition createPhoneField(const QString& id, const QString& label, bool required = false);
};

using FieldDefinitionList = QList<FieldDefinition>;

} // namespace Ballot::Core::Models

Q_DECLARE_METATYPE(Ballot::Core::Models::FieldDefinition)