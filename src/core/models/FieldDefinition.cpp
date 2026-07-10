#include "FieldDefinition.h"
#include <QJsonDocument>
#include <QUuid>

namespace Ballot::Core::Models {

QJsonObject FieldConstraint::toJson() const {
    QJsonObject obj;
    obj["type"] = static_cast<int>(type);
    obj["value"] = QJsonValue::fromVariant(value);
    obj["errorMessage"] = errorMessage;
    return obj;
}

FieldConstraint FieldConstraint::fromJson(const QJsonObject& obj) {
    FieldConstraint constraint;
    constraint.type = static_cast<FieldValidation>(obj["type"].toInt());
    constraint.value = obj["value"].toVariant();
    constraint.errorMessage = obj["errorMessage"].toString();
    return constraint;
}

QJsonObject FieldOption::toJson() const {
    QJsonObject obj;
    obj["value"] = value;
    obj["label"] = label;
    obj["description"] = description;
    obj["metadata"] = QJsonValue::fromVariant(metadata);
    obj["isDefault"] = isDefault;
    obj["sortOrder"] = sortOrder;
    return obj;
}

FieldOption FieldOption::fromJson(const QJsonObject& obj) {
    FieldOption option;
    option.value = obj["value"].toString();
    option.label = obj["label"].toString();
    option.description = obj["description"].toString();
    option.metadata = obj["metadata"].toVariant();
    option.isDefault = obj["isDefault"].toBool();
    option.sortOrder = obj["sortOrder"].toInt();
    return option;
}

QJsonObject FieldConditional::toJson() const {
    QJsonObject obj;
    obj["dependsOnField"] = dependsOnField;
    obj["operatorType"] = operatorType;
    obj["value"] = QJsonValue::fromVariant(value);
    obj["visibility"] = static_cast<int>(visibility);
    return obj;
}

FieldConditional FieldConditional::fromJson(const QJsonObject& obj) {
    FieldConditional conditional;
    conditional.dependsOnField = obj["dependsOnField"].toString();
    conditional.operatorType = obj["operatorType"].toString();
    conditional.value = obj["value"].toVariant();
    conditional.visibility = static_cast<FieldVisibility>(obj["visibility"].toInt());
    return conditional;
}

QJsonObject FieldDefinition::toJson() const {
    QJsonObject obj;
    obj["id"] = id;
    obj["name"] = name;
    obj["label"] = label;
    obj["placeholder"] = placeholder;
    obj["helpText"] = helpText;
    obj["groupName"] = groupName;
    obj["category"] = category;
    obj["type"] = static_cast<int>(type);
    obj["defaultValue"] = QJsonValue::fromVariant(defaultValue);
    obj["isRequired"] = isRequired;
    obj["isUnique"] = isUnique;
    obj["isIndexed"] = isIndexed;
    obj["isSearchable"] = isSearchable;
    obj["isSortable"] = isSortable;
    obj["isExportable"] = isExportable;
    obj["isEditable"] = isEditable;
    obj["isVisible"] = isVisible;
    obj["readOnly"] = readOnly;
    obj["minLength"] = minLength;
    obj["maxLength"] = maxLength;
    obj["minValue"] = minValue;
    obj["maxValue"] = maxValue;
    obj["step"] = step;
    obj["pattern"] = pattern;
    obj["customValidation"] = customValidation;
    
    QJsonArray optionsArray;
    for (const auto& option : options) {
        optionsArray.append(option.toJson());
    }
    obj["options"] = optionsArray;
    
    QJsonArray constraintsArray;
    for (const auto& constraint : constraints) {
        constraintsArray.append(constraint.toJson());
    }
    obj["constraints"] = constraintsArray;
    
    QJsonArray conditionalsArray;
    for (const auto& conditional : conditionals) {
        conditionalsArray.append(conditional.toJson());
    }
    obj["conditionals"] = conditionalsArray;
    
    obj["sortOrder"] = sortOrder;
    obj["cssClass"] = cssClass;
    obj["icon"] = icon;
    obj["metadata"] = QJsonValue::fromVariant(metadata);
    return obj;
}

FieldDefinition FieldDefinition::fromJson(const QJsonObject& obj) {
    FieldDefinition field;
    field.id = obj["id"].toString();
    field.name = obj["name"].toString();
    field.label = obj["label"].toString();
    field.placeholder = obj["placeholder"].toString();
    field.helpText = obj["helpText"].toString();
    field.groupName = obj["groupName"].toString();
    field.category = obj["category"].toString();
    field.type = static_cast<FieldType>(obj["type"].toInt());
    field.defaultValue = obj["defaultValue"].toVariant();
    field.isRequired = obj["isRequired"].toBool();
    field.isUnique = obj["isUnique"].toBool();
    field.isIndexed = obj["isIndexed"].toBool();
    field.isSearchable = obj["isSearchable"].toBool();
    field.isSortable = obj["isSortable"].toBool();
    field.isExportable = obj["isExportable"].toBool();
    field.isEditable = obj["isEditable"].toBool();
    field.isVisible = obj["isVisible"].toBool();
    field.readOnly = obj["readOnly"].toBool();
    field.minLength = obj["minLength"].toInt();
    field.maxLength = obj["maxLength"].toInt();
    field.minValue = obj["minValue"].toDouble();
    field.maxValue = obj["maxValue"].toDouble();
    field.step = obj["step"].toInt();
    field.pattern = obj["pattern"].toString();
    field.customValidation = obj["customValidation"].toString();
    
    QJsonArray optionsArray = obj["options"].toArray();
    for (const auto& opt : optionsArray) {
        field.options.append(FieldOption::fromJson(opt.toObject()));
    }
    
    QJsonArray constraintsArray = obj["constraints"].toArray();
    for (const auto& cons : constraintsArray) {
        field.constraints.append(FieldConstraint::fromJson(cons.toObject()));
    }
    
    QJsonArray conditionalsArray = obj["conditionals"].toArray();
    for (const auto& cond : conditionalsArray) {
        field.conditionals.append(FieldConditional::fromJson(cond.toObject()));
    }
    
    field.sortOrder = obj["sortOrder"].toInt();
    field.cssClass = obj["cssClass"].toString();
    field.icon = obj["icon"].toString();
    field.metadata = obj["metadata"].toVariant().toMap();
    return field;
}

FieldDefinition FieldDefinition::createTextField(const QString& id, const QString& label, bool required) {
    FieldDefinition field;
    field.id = id.isEmpty() ? QUuid::createUuid().toString(QUuid::WithoutBraces) : id;
    field.name = id;
    field.label = label;
    field.type = FieldType::Text;
    field.isRequired = required;
    field.isSearchable = true;
    field.isSortable = true;
    field.maxLength = 255;
    return field;
}

FieldDefinition FieldDefinition::createNumberField(const QString& id, const QString& label, bool required) {
    FieldDefinition field;
    field.id = id.isEmpty() ? QUuid::createUuid().toString(QUuid::WithoutBraces) : id;
    field.name = id;
    field.label = label;
    field.type = FieldType::Number;
    field.isRequired = required;
    field.isSearchable = true;
    field.isSortable = true;
    field.minValue = 0;
    field.maxValue = 999999999;
    return field;
}

FieldDefinition FieldDefinition::createSelectField(const QString& id, const QString& label, const QList<FieldOption>& options, bool required) {
    FieldDefinition field;
    field.id = id.isEmpty() ? QUuid::createUuid().toString(QUuid::WithoutBraces) : id;
    field.name = id;
    field.label = label;
    field.type = FieldType::SelectSingle;
    field.isRequired = required;
    field.isSearchable = true;
    field.isSortable = true;
    field.options = options;
    return field;
}

FieldDefinition FieldDefinition::createDateField(const QString& id, const QString& label, bool required) {
    FieldDefinition field;
    field.id = id.isEmpty() ? QUuid::createUuid().toString(QUuid::WithoutBraces) : id;
    field.name = id;
    field.label = label;
    field.type = FieldType::Date;
    field.isRequired = required;
    field.isSearchable = true;
    field.isSortable = true;
    return field;
}

FieldDefinition FieldDefinition::createImageField(const QString& id, const QString& label, bool required) {
    FieldDefinition field;
    field.id = id.isEmpty() ? QUuid::createUuid().toString(QUuid::WithoutBraces) : id;
    field.name = id;
    field.label = label;
    field.type = FieldType::Image;
    field.isRequired = required;
    field.isSearchable = false;
    field.isSortable = false;
    field.isExportable = false;
    return field;
}

FieldDefinition FieldDefinition::createEmailField(const QString& id, const QString& label, bool required) {
    FieldDefinition field = createTextField(id, label, required);
    field.type = FieldType::Email;
    field.pattern = "^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$";
    field.constraints.append(FieldConstraint{FieldValidation::Email, QVariant(), "Please enter a valid email address"});
    return field;
}

FieldDefinition FieldDefinition::createPhoneField(const QString& id, const QString& label, bool required) {
    FieldDefinition field = createTextField(id, label, required);
    field.type = FieldType::Phone;
    field.pattern = "^[+]?[(]?[0-9]{1,3}[)]?[-\\s.]?[(]?[0-9]{1,3}[)]?[-\\s.]?[0-9]{4,6}$";
    field.constraints.append(FieldConstraint{FieldValidation::Phone, QVariant(), "Please enter a valid phone number"});
    return field;
}

} // namespace Ballot::Core::Models
