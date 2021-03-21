#include <serializabledataobject.h>

void SerializableDataObject::simpleValuesFromJsonObject(
    const QJsonObject &content) {
  auto metaObject = this->metaObject();
  for (QString key : content.keys()) {
    std::string keyString = key.toStdString();
    const char *keyChars = keyString.c_str();
    // Find index of property
    int propIndex = metaObject->indexOfProperty(keyChars);
    if (propIndex < 0) {
      continue;
    }

    QJsonValue jsonValue = content.value(key);
    if (jsonValue.isBool() || jsonValue.isDouble() || jsonValue.isString()) {
      QVariant variant = this->property(keyChars);
      this->setProperty(key.toLatin1().data(), jsonValue.toVariant());
    }
  }
}

QJsonObject SerializableDataObject::recursiveToJsonObject() const {
  QJsonObject jsonObject;
  auto metaObject = this->metaObject();
  // TODO metaobject->superclass auch für weitere vererbung
  for (int i = metaObject->superClass()->propertyOffset() - 1;
       i < metaObject->propertyCount(); ++i) {
    QMetaProperty prop = metaObject->property(i);
    QVariant variant = property(prop.name());

    if (variant.canConvert<SerializableDataObject *>()) {
      SerializableDataObject *sub_obj =
          variant.value<SerializableDataObject *>();
      jsonObject.insert(prop.name(), sub_obj->toJsonObject());

    } else if (variant.canConvert<QList<QVariant>>()) {
      jsonObject.insert(prop.name(),
                        toJsonArray(variant.value<QList<QVariant>>()));

    } else {
      jsonObject.insert(prop.name(), QJsonValue::fromVariant(variant));
    }
  }

  return jsonObject;
}

QJsonArray SerializableDataObject::toJsonArray(
    const QList<SerializableDataObject *> &list) const {
  if (list.size() > 0) {
    if (list.front()->parent() == this) {
      return toObjectJsonArray(list);
    } else {
      return toIdJsonArray(list);
    }
  } else {
    return QJsonArray();
  }
}

QJsonArray
SerializableDataObject::toJsonArray(const QList<QVariant> &list) const {
  if (list.size() > 0) {
    QVariant firstEntry = list.front();

    // If QList contains SerializableDataObjects
    if (firstEntry.canConvert<SerializableDataObject *>()) {
      // TODO Continue looking for a nicer way to convert QList<QVariant> to
      // QList<SerializableDataObject>
      QList<SerializableDataObject *> sdoPointerList;
      for (QVariant variant : list) {
        // Only allow one reference to each object in a list
        // TODO If creating an id list multiple references to the same object
        // should be ok
        SerializableDataObject *sdoPointer =
            variant.value<SerializableDataObject *>();
        if (!sdoPointerList.contains(sdoPointer)) {
          sdoPointerList.append(sdoPointer);
        }
      }
      return toJsonArray(sdoPointerList);
    }

    // Use default JsonValue fromVariant if no SerializableDataObject
    // TODO check other types, that cannot be converted directly
    QJsonArray returnArray;
    for (QVariant variant : list) {
      returnArray.append(QJsonValue::fromVariant(variant));
    }
    return returnArray;
  }
  return QJsonArray();
}

QJsonArray SerializableDataObject::toObjectJsonArray(
    const QList<SerializableDataObject *> &list) const {
  QJsonArray activeGroupArray;
  for (SerializableDataObject *sdo : list) {
    activeGroupArray.push_back(sdo->toJsonObject());
  }
  return activeGroupArray;
}

QJsonArray SerializableDataObject::toIdJsonArray(
    const QList<SerializableDataObject *> &list) const {
  QJsonArray activeGroupArray;
  for (SerializableDataObject *sdo : list) {
    activeGroupArray.push_back(sdo->id.toString());
  }
  return activeGroupArray;
}

bool SerializableDataObject::setPropertyValue(const QJsonValue &value,
                                              const QString &propertyName) {
  auto metaObject = this->metaObject();
  std::string propertyNameString = propertyName.toStdString();
  const char *propertyNameChars = propertyNameString.c_str();
  int propertyIndex = metaObject->indexOfProperty(propertyNameChars);

  // Return false if property does not exist
  if (propertyIndex < 0) {
    return false;
  }

  // TODO remove if really unneeded
  // QMetaProperty property = metaObject->property(propertyIndex);
  // QMetaType::Type propertyType = (QMetaType::Type) property.type();

  // TODO QVariant::Type not completly equal to QMetaType::Type. Check the
  // differences and account for them later
  QJsonValue::Type jsonType = value.type();

  switch (jsonType) {
  case QJsonValue::Null:
  case QJsonValue::Bool:
  case QJsonValue::Double:
  case QJsonValue::String:
    // qDebug() << "Setting " << propertyNameChars << " to " <<
    // value.toVariant() << " (" << value.toVariant().typeName() << ")";
    return this->setProperty(propertyNameChars, value.toVariant());
    break;
  case QJsonValue::Object:

    break;
  case QJsonValue::Array:
    break;
  case QJsonValue::Undefined:
    return false;
  }

  return false;
}

QMetaType::Type
SerializableDataObject::getTypeFromList(const QMetaType::Type &listType) const {
  const char *listTypeName = QMetaType::typeName(listType);
  const char *contentStart = strchr(listTypeName, '<') + 1;
  const char *contentEnd = strrchr(listTypeName, '>');
  size_t contentLength = contentEnd - contentStart;
  if (contentStart == nullptr || contentEnd == nullptr || contentLength <= 0) {
    return QMetaType::UnknownType;
  }

  char *contentTypeName = new char[contentLength + 1]();
  memcpy(contentTypeName, contentStart, contentLength);

  return (QMetaType::Type)QMetaType::type(contentTypeName);
}

QVariant SerializableDataObject::createListFromValueAndListType(
    const QJsonArray &value, const QMetaType::Type &listType) {
  QMetaType::Type contentType = getTypeFromList(listType);
  return createListFromValueAndContentType(value, contentType);
}

QVariant SerializableDataObject::createListFromValueAndContentType(
    const QJsonArray &value, const QMetaType::Type &listType) {
  QMetaType::Type contentType = getTypeFromList(listType);
  if (contentType == QMetaType::UnknownType) {
    return QList<QVariant>();
  }

  QMetaType::TypeFlags flags = QMetaType::typeFlags(contentType);
  if ((flags & QMetaType::PointerToQObject) != 0) {
    qDebug() << "Is pointer to QObject";
    QList<QObject *> resultList;
    const QMetaObject *metaObject = QMetaType::metaObjectForType(contentType);
    for (QJsonValue arrayEntry : value) {
      QObject *qObject = metaObject->newInstance(Q_ARG(QObject *, this));
      resultList.push_back(qObject);

      // Test if object can be cast to a SerializableDataObject* and initialize
      // if possible
      SerializableDataObject *sdo =
          dynamic_cast<SerializableDataObject *>(qObject);
      if (sdo != nullptr) {
        sdo->fromJsonObject(arrayEntry.toObject());
      }
    }

    QVariant var((QVariant::Type)listType);
    QVariant convar;
    var.setValue(resultList);
    var.convert(listType);
    return var;
  } else {
    qDebug() << "Is no pointer to QObject";
  }

  return QList<QVariant>();
}

QUuid SerializableDataObject::getId() { return id; }

SerializableDataObject::SerializableDataObject(QObject *parent, QUuid id)
    : QObject(parent), id(id) {}
