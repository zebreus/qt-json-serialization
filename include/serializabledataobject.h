#ifndef SERIALIZABLEDATAOBJECT_H
#define SERIALIZABLEDATAOBJECT_H

class SerializableDataObject;

#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QList>
#include <QMetaProperty>
#include <QMetaType>
#include <QObject>
#include <QUuid>

class SerializableDataObject : public QObject {
  Q_OBJECT
  Q_PROPERTY(QUuid id READ getId)
public:
  virtual void fromJsonObject(const QJsonObject &content) = 0;
  virtual QJsonObject toJsonObject() const = 0;
  QUuid getId();
  explicit SerializableDataObject(QObject *parent = nullptr,
                                  QUuid id = QUuid::createUuid());

protected:
  /** Reads all strings, numbers and booleans from QJsonObject into this
   * metaobject
   *
   * @param  content the QJsonObject containing key value pairs for every
   * string, number and boolean property of this QMetaObject
   */
  virtual void simpleValuesFromJsonObject(const QJsonObject &content);

  /** Writes all strings, numbers, booleans and QList<SerializableDataObject*>
   * into a QJsonObject
   *
   * @return a QJsonObject containing all the properties of this object
   */
  virtual QJsonObject recursiveToJsonObject() const;

  /** Returns the property as JSON array of objects or ids.
   *
   */
  virtual QJsonArray
  toJsonArray(const QList<SerializableDataObject *> &list) const;
  virtual QJsonArray toJsonArray(const QList<QVariant> &list) const;
  virtual QJsonArray
  toObjectJsonArray(const QList<SerializableDataObject *> &list) const;
  virtual QJsonArray
  toIdJsonArray(const QList<SerializableDataObject *> &list) const;

  /** Creates a QList containing pointers to every sdo in content
   * For every entry in QJsonArray an object will be created
   *
   * @param  content a QJsonArray containing number QJsonObjects containing the
   * properties of the objects
   * @return
   */
  template <class T> QList<T *> fromObjectJsonArray(const QJsonArray &content);
  template <class T> QList<T *> fromObjectJsonArray(const QJsonValue &content);

  // setzt property, falls sie vorhanden ist
  bool setPropertyValue(const QJsonValue &value, const QString &propertyName);
  QMetaType::Type getTypeFromList(const QMetaType::Type &type) const;
  QVariant createListFromValueAndListType(const QJsonArray &value,
                                          const QMetaType::Type &listType);
  QVariant
  createListFromValueAndContentType(const QJsonArray &value,
                                    const QMetaType::Type &contentType);
  // Erstellt Liste aus daten und dem Name des typs der Daten die in dem Array
  // sind.
  QList<SerializableDataObject *>
  createFromJsonArrayAndName(const QJsonArray &content,
                             const QString *type) const;
  SerializableDataObject *createFromJsonAndName(const QJsonObject &content,
                                                const QString *type) const;
  SerializableDataObject *createFromJsonAndType(const QJsonObject &content,
                                                const QMetaObject *type) const;
  QMetaObject *createMetaObjectFromName(const QString &name) const;

  /** Creates a QList containing pointers to sdos from objects with the ids from
   * content No objects will be created, the returned QList will only contain
   * pointers from objects
   *
   * @param  content a QJsonArray containing number QJsonValues containing the
   * ids of the objects to refer to
   * @param  objects a QList of SerializableDataObjectss
   * @return
   */
  template <class T>
  QList<T *> fromIdJsonArray(const QJsonArray &content,
                             const QList<T *> &objects);
  template <class T>
  QList<T *> fromIdJsonArray(const QJsonValue &content,
                             const QList<T *> &objects);

  QUuid id;
};

template <class T>
QList<T *>
SerializableDataObject::fromObjectJsonArray(const QJsonArray &content) {
  QList<T *> resultList;

  for (QJsonValue jsonObjectValue : content) {
    QJsonObject jsonObject = jsonObjectValue.toObject();
    QUuid id = jsonObject.value("id").toString();
    T *sdoPointer = new T(this, id);
    sdoPointer->fromJsonObject(jsonObject);
    resultList.append(sdoPointer);
  }

  return resultList;
}
template <class T>
QList<T *>
SerializableDataObject::fromObjectJsonArray(const QJsonValue &content) {
  return fromObjectJsonArray<T>(content.toArray());
}

template <class T>
QList<T *> SerializableDataObject::fromIdJsonArray(const QJsonArray &content,
                                                   const QList<T *> &objects) {
  QList<T *> resultList;

  for (QJsonValue contentId : content) {
    QUuid contentIdInt = contentId.toString();

    // Find the object with matching id and add reference to modules
    for (T *sdoPointer : objects) {
      if (sdoPointer->getId() == contentIdInt) {
        resultList.push_back(sdoPointer);
      }
    }
  }

  return resultList;
}
template <class T>
QList<T *> SerializableDataObject::fromIdJsonArray(const QJsonValue &content,
                                                   const QList<T *> &objects) {
  return fromIdJsonArray<T>(content.toArray(), objects);
}

#endif // SERIALIZABLEDATAOBJECT_H
