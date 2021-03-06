/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

// NOTE: Don't add dependencies to other files.
// This is used in the debugger auto-tests.

#include "watchdata.h"
#include "watchutils.h"
#include "debuggerprotocol.h"

#include <QDebug>

namespace Debugger {
namespace Internal {

bool isPointerType(const QByteArray &type)
{
    return type.endsWith('*') || type.endsWith("* const");
}

bool isCharPointerType(const QByteArray &type)
{
    return type == "char *" || type == "const char *" || type == "char const *";
}

bool isIntType(const QByteArray &type)
{
    if (type.isEmpty())
        return false;
    switch (type.at(0)) {
        case 'b':
            return type == "bool";
        case 'c':
            return type == "char";
        case 'i':
            return type == "int";
        case 'l':
            return type == "long"
                || type == "long int"
                || type == "long unsigned int";
        case 'p':
            return type == "ptrdiff_t";
        case 'q':
            return type == "qint16" || type == "quint16"
                || type == "qint32" || type == "quint32"
                || type == "qint64" || type == "quint64"
                || type == "qlonglong" || type == "qulonglong";
        case 's':
            return type == "short"
                || type == "signed"
                || type == "size_t"
                || type == "std::size_t"
                || type == "std::ptrdiff_t"
                || (type.startsWith("signed") &&
                    (  type == "signed char"
                    || type == "signed short"
                    || type == "signed short int"
                    || type == "signed long"
                    || type == "signed long int"
                    || type == "signed long long"
                    || type == "signed long long int"));
        case 'u':
            return type == "unsigned"
                || (type.startsWith("unsigned") &&
                    (  type == "unsigned char"
                    || type == "unsigned short"
                    || type == "unsigned short int"
                    || type == "unsigned int"
                    || type == "unsigned long"
                    || type == "unsigned long int"
                    || type == "unsigned long long"
                    || type == "unsigned long long int"));
        default:
            return false;
    }
}

bool isFloatType(const QByteArray &type)
{
    return type == "float" || type == "double" || type == "qreal";
}

bool isIntOrFloatType(const QByteArray &type)
{
    return isIntType(type) || isFloatType(type);
}

WatchItem::WatchItem() :
    id(WatchItem::InvalidId),
    state(InitialState),
    editformat(StopDisplay),
    address(0),
    origaddr(0),
    size(0),
    bitpos(0),
    bitsize(0),
    elided(0),
    arrayIndex(-1),
    wantsChildren(false),
    valueEnabled(true),
    valueEditable(true),
    outdated(false)
{
}

bool WatchItem::isVTablePointer() const
{
    // First case: Cdb only. No user type can be named like this, this is safe.
    // Second case: Python dumper only.
    return type.startsWith("__fptr()")
        || (type.isEmpty() && name == QLatin1String("[vptr]"));
}

void WatchItem::setError(const QString &msg)
{
    setAllUnneeded();
    value = msg;
    wantsChildren = false;
    valueEnabled = false;
    valueEditable = false;
}

void WatchItem::setValue(const QString &value0)
{
    value = value0;
    if (value == QLatin1String("{...}")) {
        value.clear();
        wantsChildren = true; // at least one...
    }
    // strip off quoted characters for chars.
    if (value.endsWith(QLatin1Char('\'')) && type.endsWith("char")) {
        const int blankPos = value.indexOf(QLatin1Char(' '));
        if (blankPos != -1)
            value.truncate(blankPos);
    }

    // avoid duplicated information
    if (value.startsWith(QLatin1Char('(')) && value.contains(QLatin1String(") 0x")))
        value.remove(0, value.lastIndexOf(QLatin1String(") 0x")) + 2);

    // doubles are sometimes displayed as "@0x6141378: 1.2".
    // I don't want that.
    if (/*isIntOrFloatType(type) && */ value.startsWith(QLatin1String("@0x"))
         && value.contains(QLatin1Char(':'))) {
        value.remove(0, value.indexOf(QLatin1Char(':')) + 2);
        setHasChildren(false);
    }

    // "numchild" is sometimes lying
    //MODEL_DEBUG("\n\n\nPOINTER: " << type << value);
    if (isPointerType(type))
        setHasChildren(value != QLatin1String("0x0") && value != QLatin1String("<null>")
            && !isCharPointerType(type));

    // pointer type information is available in the 'type'
    // column. No need to duplicate it here.
    if (value.startsWith(QLatin1Char('(') + QLatin1String(type) + QLatin1String(") 0x")))
        value = value.section(QLatin1Char(' '), -1, -1);

    setValueUnneeded();
}

enum GuessChildrenResult { HasChildren, HasNoChildren, HasPossiblyChildren };

static GuessChildrenResult guessChildren(const QByteArray &type)
{
    if (isIntOrFloatType(type))
        return HasNoChildren;
    if (isCharPointerType(type))
        return HasNoChildren;
    if (isPointerType(type))
        return HasChildren;
    if (type.endsWith("QString"))
        return HasNoChildren;
    return HasPossiblyChildren;
}

void WatchItem::setType(const QByteArray &str, bool guessChildrenFromType)
{
    type = str.trimmed();
    bool changed = true;
    while (changed) {
        if (type.endsWith("const"))
            type.chop(5);
        else if (type.endsWith(' '))
            type.chop(1);
        else if (type.startsWith("const "))
            type = type.mid(6);
        else if (type.startsWith("volatile "))
            type = type.mid(9);
        else if (type.startsWith("class "))
            type = type.mid(6);
        else if (type.startsWith("struct "))
            type = type.mid(6);
        else if (type.startsWith(' '))
            type = type.mid(1);
        else
            changed = false;
    }
    if (guessChildrenFromType) {
        switch (guessChildren(type)) {
        case HasChildren:
            setHasChildren(true);
            break;
        case HasNoChildren:
            setHasChildren(false);
            break;
        case HasPossiblyChildren:
            setHasChildren(true); // FIXME: bold assumption
            break;
        }
    }
}

QString WatchItem::toString() const
{
    const char *doubleQuoteComma = "\",";
    QString res;
    QTextStream str(&res);
    str << QLatin1Char('{');
    if (!iname.isEmpty())
        str << "iname=\"" << iname << doubleQuoteComma;
    if (!name.isEmpty() && name != QLatin1String(iname))
        str << "name=\"" << name << doubleQuoteComma;
    if (address) {
        str.setIntegerBase(16);
        str << "addr=\"0x" << address << doubleQuoteComma;
        str.setIntegerBase(10);
    }
    if (origaddr) {
        str.setIntegerBase(16);
        str << "referencingaddr=\"0x" << origaddr << doubleQuoteComma;
        str.setIntegerBase(10);
    }
    if (!exp.isEmpty())
        str << "exp=\"" << exp << doubleQuoteComma;

    if (isValueNeeded())
        str << "value=<needed>,";
    if (!value.isEmpty())
        str << "value=\"" << value << doubleQuoteComma;

    if (elided)
        str << "valueelided=\"" << elided << doubleQuoteComma;

    if (!editvalue.isEmpty())
        str << "editvalue=\"<...>\",";

    str << "type=\"" << type << doubleQuoteComma;

    str << "wantsChildren=\"" << (wantsChildren ? "true" : "false") << doubleQuoteComma;

    if (isChildrenNeeded())
        str << "children=<needed>,";
    str.flush();
    if (res.endsWith(QLatin1Char(',')))
        res.truncate(res.size() - 1);
    return res + QLatin1Char('}');
}

QString WatchItem::msgNotInScope()
{
    //: Value of variable in Debugger Locals display for variables out
    //: of scope (stopped above initialization).
    static const QString rc =
        QCoreApplication::translate("Debugger::Internal::WatchItem", "<not in scope>");
    return rc;
}

const QString &WatchItem::shadowedNameFormat()
{
    //: Display of variables shadowed by variables of the same name
    //: in nested scopes: Variable %1 is the variable name, %2 is a
    //: simple count.
    static const QString format =
        QCoreApplication::translate("Debugger::Internal::WatchItem", "%1 <shadowed %2>");
    return format;
}

QString WatchItem::shadowedName(const QString &name, int seen)
{
    if (seen <= 0)
        return name;
    return shadowedNameFormat().arg(name).arg(seen);
}

QByteArray WatchItem::hexAddress() const
{
    if (address)
        return QByteArray("0x") + QByteArray::number(address, 16);
    return QByteArray();
}

template <class T>
QString decodeItemHelper(const T &t)
{
    return QString::number(t);
}

QString decodeItemHelper(const double &t)
{
    return QString::number(t, 'g', 16);
}

template <class T>
void decodeArrayHelper(WatchItem *item, const QByteArray &rawData, int size, const QByteArray &childType)
{
    const QByteArray ba = QByteArray::fromHex(rawData);
    const T *p = (const T *) ba.data();
    for (int i = 0, n = ba.size() / sizeof(T); i < n; ++i) {
        WatchItem *child = new WatchItem;
        child->arrayIndex = i;
        child->value = decodeItemHelper(p[i]);
        child->size = size;
        child->type = childType;
        child->setAllUnneeded();
        item->appendChild(child);
    }
}

static void decodeArrayData(WatchItem *item, const QByteArray &rawData,
    const DebuggerEncoding &encoding, const QByteArray &childType)
{
    switch (encoding.type) {
        case DebuggerEncoding::HexEncodedSignedInteger:
            switch (encoding.size) {
                case 1:
                    return decodeArrayHelper<signed char>(item, rawData, encoding.size, childType);
                case 2:
                    return decodeArrayHelper<short>(item, rawData, encoding.size, childType);
                case 4:
                    return decodeArrayHelper<int>(item, rawData, encoding.size, childType);
                case 8:
                    return decodeArrayHelper<qint64>(item, rawData, encoding.size, childType);
            }
        case DebuggerEncoding::HexEncodedUnsignedInteger:
            switch (encoding.size) {
                case 1:
                    return decodeArrayHelper<uchar>(item, rawData, encoding.size, childType);
                case 2:
                    return decodeArrayHelper<ushort>(item, rawData, encoding.size, childType);
                case 4:
                    return decodeArrayHelper<uint>(item, rawData, encoding.size, childType);
                case 8:
                    return decodeArrayHelper<quint64>(item, rawData, encoding.size, childType);
            }
            break;
        case DebuggerEncoding::HexEncodedFloat:
            switch (encoding.size) {
                case 4:
                    return decodeArrayHelper<float>(item, rawData, encoding.size, childType);
                case 8:
                    return decodeArrayHelper<double>(item, rawData, encoding.size, childType);
            }
        default:
            break;
    }
    qDebug() << "ENCODING ERROR: " << encoding.toString();
}

void WatchItem::parseHelper(const GdbMi &input)
{
    setChildrenUnneeded();

    GdbMi mi = input["type"];
    if (mi.isValid())
        setType(mi.data());

    editvalue = input["editvalue"].data();
    editformat = DebuggerDisplay(input["editformat"].toInt());
    editencoding = DebuggerEncoding(input["editencoding"].data());

    mi = input["valueelided"];
    if (mi.isValid())
        elided = mi.toInt();

    mi = input["bitpos"];
    if (mi.isValid())
        bitpos = mi.toInt();

    mi = input["bitsize"];
    if (mi.isValid())
        bitsize = mi.toInt();

    mi = input["origaddr"];
    if (mi.isValid())
        origaddr = mi.toAddress();

    mi = input["address"];
    if (mi.isValid()) {
        address = mi.toAddress();
        if (exp.isEmpty()) {
            if (iname.startsWith("local.") && iname.count('.') == 1)
                // Solve one common case of adding 'class' in
                // *(class X*)0xdeadbeef for gdb.
                exp = name.toLatin1();
            else
                exp = "*(" + gdbQuoteTypes(type) + "*)" + hexAddress();
        }
    }

    mi = input["value"];
    QByteArray enc = input["valueencoded"].data();
    if (mi.isValid() || !enc.isEmpty()) {
        setValue(decodeData(mi.data(), enc));
    } else {
        setValueNeeded();
    }

    mi = input["size"];
    if (mi.isValid())
        size = mi.toInt();

    mi = input["exp"];
    if (mi.isValid())
        exp = mi.data();

    mi = input["valueenabled"];
    if (mi.data() == "true")
        valueEnabled = true;
    else if (mi.data() == "false")
        valueEnabled = false;

    mi = input["valueeditable"];
    if (mi.data() == "true")
        valueEditable = true;
    else if (mi.data() == "false")
        valueEditable = false;

    mi = input["numchild"]; // GDB/MI
    if (mi.isValid())
        setHasChildren(mi.toInt() > 0);
    mi = input["haschild"]; // native-mixed
    if (mi.isValid())
        setHasChildren(mi.toInt() > 0);

    mi = input["arraydata"];
    if (mi.isValid()) {
        DebuggerEncoding encoding(input["arrayencoding"].data());
        QByteArray childType = input["childtype"].data();
        decodeArrayData(this, mi.data(), encoding, childType);
    } else {
        const GdbMi children = input["children"];
        if (children.isValid()) {
            bool ok = false;
            // Try not to repeat data too often.
            const GdbMi childType = input["childtype"];
            const GdbMi childNumChild = input["childnumchild"];

            qulonglong addressBase = input["addrbase"].data().toULongLong(&ok, 0);
            qulonglong addressStep = input["addrstep"].data().toULongLong(&ok, 0);

            for (int i = 0, n = int(children.children().size()); i != n; ++i) {
                const GdbMi &subinput = children.children().at(i);
                WatchItem *child = new WatchItem;
                if (childType.isValid())
                    child->setType(childType.data());
                if (childNumChild.isValid())
                    child->setHasChildren(childNumChild.toInt() > 0);
                GdbMi name = subinput["name"];
                QByteArray nn;
                if (name.isValid()) {
                    nn = name.data();
                    child->name = QString::fromLatin1(nn);
                } else {
                    nn.setNum(i);
                    child->name = QString::fromLatin1("[%1]").arg(i);
                }
                GdbMi iname = subinput["iname"];
                if (iname.isValid())
                    child->iname = iname.data();
                else
                    child->iname = this->iname + '.' + nn;
                if (addressStep) {
                    child->address = addressBase + i * addressStep;
                    child->exp = "*(" + gdbQuoteTypes(child->type) + "*)" + child->hexAddress();
                }
                QByteArray key = subinput["key"].data();
                if (!key.isEmpty())
                    child->name = decodeData(key, subinput["keyencoded"].data());
                child->parseHelper(subinput);
                appendChild(child);
            }
        }
    }
}

void WatchItem::parse(const GdbMi &data)
{
    iname = data["iname"].data();

    GdbMi wname = data["wname"];
    if (wname.isValid()) // Happens (only) for watched expressions.
        name = QString::fromUtf8(QByteArray::fromHex(wname.data()));
    else
        name = QString::fromLatin1(data["name"].data());

    parseHelper(data);

    if (wname.isValid())
        exp = name.toUtf8();
}

WatchItem *WatchItem::parentItem() const
{
    return static_cast<WatchItem *>(parent());
}

// Format a tooltip row with aligned colon.
static void formatToolTipRow(QTextStream &str, const QString &category, const QString &value)
{
    QString val = value.toHtmlEscaped();
    val.replace(QLatin1Char('\n'), QLatin1String("<br>"));
    str << "<tr><td>" << category << "</td><td>";
    if (!category.isEmpty())
        str << ':';
    str << "</td><td>" << val << "</td></tr>";
}

QString WatchItem::toToolTip() const
{
    QString res;
    QTextStream str(&res);
    str << "<html><body><table>";
    formatToolTipRow(str, tr("Name"), name);
    formatToolTipRow(str, tr("Expression"), expression());
    formatToolTipRow(str, tr("Internal Type"), QLatin1String(type));
    bool ok;
    const quint64 intValue = value.toULongLong(&ok);
    if (ok && intValue) {
        formatToolTipRow(str, tr("Value"), QLatin1String("(dec)  ") + value);
        formatToolTipRow(str, QString(), QLatin1String("(hex)  ") + QString::number(intValue, 16));
        formatToolTipRow(str, QString(), QLatin1String("(oct)  ") + QString::number(intValue, 8));
        formatToolTipRow(str, QString(), QLatin1String("(bin)  ") + QString::number(intValue, 2));
    } else {
        QString val = value;
        if (val.size() > 1000) {
            val.truncate(1000);
            val += QLatin1Char(' ');
            val += tr("... <cut off>");
        }
        formatToolTipRow(str, tr("Value"), val);
    }
    if (realAddress())
        formatToolTipRow(str, tr("Object Address"), formatToolTipAddress(realAddress()));
    if (origaddr)
        formatToolTipRow(str, tr("Pointer Address"), formatToolTipAddress(origaddr));
    if (arrayIndex >= 0)
        formatToolTipRow(str, tr("Array Index"), QString::number(arrayIndex));
    if (size)
        formatToolTipRow(str, tr("Static Object Size"), tr("%n bytes", 0, size));
    formatToolTipRow(str, tr("Internal ID"), QLatin1String(internalName()));
    str << "</table></body></html>";
    return res;
}

bool WatchItem::isLocal() const
{
    if (arrayIndex >= 0)
        if (const WatchItem *p = parentItem())
            return p->isLocal();
    return iname.startsWith("local.");
}

bool WatchItem::isWatcher() const
{
    if (arrayIndex >= 0)
        if (const WatchItem *p = parentItem())
            return p->isWatcher();
    return iname.startsWith("watch.");
}

bool WatchItem::isInspect() const
{
    if (arrayIndex >= 0)
        if (const WatchItem *p = parentItem())
            return p->isInspect();
    return iname.startsWith("inspect.");
}

quint64 WatchItem::realAddress() const
{
    if (arrayIndex >= 0) {
        if (const WatchItem *p = parentItem())
            return p->address + arrayIndex * size;
    }
    return address;
}

QByteArray WatchItem::internalName() const
{
    if (arrayIndex >= 0) {
        if (const WatchItem *p = parentItem())
            return p->iname + '.' + QByteArray::number(arrayIndex);
    }
    return iname;
}

QString WatchItem::realName() const
{
    if (arrayIndex >= 0)
        return QString::fromLatin1("[%1]").arg(arrayIndex);
    return name;
}

QString WatchItem::expression() const
{
    if (!exp.isEmpty())
         return QString::fromLatin1(exp);
    if (quint64 addr = realAddress()) {
        if (!type.isEmpty())
            return QString::fromLatin1("*(%1*)0x%2").arg(QLatin1String(type)).arg(addr, 0, 16);
    }
    const WatchItem *p = parentItem();
    if (p && !p->exp.isEmpty())
        return QString::fromLatin1("(%1).%2").arg(QString::fromLatin1(p->exp), name);
    return name;
}

WatchItem *WatchItem::findItem(const QByteArray &iname)
{
    if (internalName() == iname)
        return this;
    foreach (TreeItem *child, children()) {
        auto witem = static_cast<WatchItem *>(child);
        if (WatchItem *result = witem->findItem(iname))
            return result;
    }
    return 0;
}

} // namespace Internal
} // namespace Debugger

