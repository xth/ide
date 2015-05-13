/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at
** http://www.qt.io/contact-us
**
** This file is part of the Qt Creator Enterprise Auto Test Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.
**
** If you have questions regarding the use of this file, please use
** contact form at http://www.qt.io/contact-us
**
****************************************************************************/

#include "squishxmloutputhandler.h"
#include "testresultspane.h"

#include <utils/qtcassert.h>

#include <QDebug>
#include <QDateTime>
#include <QFile>
#include <QXmlStreamWriter>

namespace Autotest {
namespace Internal {

SquishXmlOutputHandler::SquishXmlOutputHandler(QObject *parent) : QObject(parent)
{
    connect(this, &SquishXmlOutputHandler::testResultCreated,
            TestResultsPane::instance(), &TestResultsPane::addTestResult, Qt::QueuedConnection);
}

void SquishXmlOutputHandler::clearForNextRun()
{
    m_xmlReader.clear();
}

void SquishXmlOutputHandler::mergeResultFiles(const QStringList &reportFiles,
                                              const QString &resultsDirectory,
                                              const QString &suiteName, QString *errorMessage)
{
    QFile resultsXML(QString::fromLatin1("%1/results.xml").arg(resultsDirectory));
    if (resultsXML.exists()) {
        if (errorMessage)
            *errorMessage = tr("Could not merge results into single results.xml.\n"
                               "Destination file \"%1\" already exists.").arg(resultsXML.fileName());
        return;
    }

    if (!resultsXML.open(QFile::WriteOnly)) {
        if (errorMessage)
            *errorMessage = tr("Could not merge results into single results.xml.\n"
                               "Failed to open file \"%1\"").arg(resultsXML.fileName());
        return;
    }

    QXmlStreamWriter xmlWriter(&resultsXML);
    xmlWriter.writeStartDocument(QLatin1String("1.0"));
    bool isFirstReport = true;
    bool isFirstTest = true;
    QString lastEpilogTime;
    foreach (const QString &caseResult, reportFiles) {
        QFile currentResultsFile(caseResult);
        if (!currentResultsFile.exists())
            continue;
        if (!currentResultsFile.open(QFile::ReadOnly))
            continue;
        QXmlStreamReader reader(&currentResultsFile);
        while (!reader.atEnd()) {
            QXmlStreamReader::TokenType type = reader.readNext();
            switch (type) {
            case QXmlStreamReader::StartElement: {
                const QStringRef tagName = reader.name();
                // SquishReport of the first results.xml will be taken as is and as this is a
                // merged results.xml we add another test tag holding the suite's name
                if (tagName == QLatin1String("SquishReport")) {
                    if (isFirstReport) {
                        xmlWriter.writeStartElement(tagName.toString());
                        xmlWriter.writeAttributes(reader.attributes());
                        xmlWriter.writeStartElement(QLatin1String("test"));
                        xmlWriter.writeAttribute(QLatin1String("name"), suiteName);
                        isFirstReport = false;
                    }
                    break;
                }
                if (isFirstTest && tagName == QLatin1String("test")) {
                    // the prolog tag of the first results.xml holds the start time of the suite
                    // we already wrote the test tag for the suite, but haven't added the start
                    // time as we didn't know about it, so store information of the current test
                    // tag (case name), read ahead (prolog tag), write prolog (suite's test tag)
                    // and finally write test tag (case name) - the prolog tag (for test case)
                    // will be written outside the if
                    const QXmlStreamAttributes testAttributes = reader.attributes();
                    QXmlStreamReader::TokenType token;
                    while (!reader.atEnd()) {
                        token = reader.readNext();
                        if (token != QXmlStreamReader::Characters)
                            break;
                    }
                    const QStringRef prolog = reader.name();
                    QTC_ASSERT(token == QXmlStreamReader::StartElement
                               && prolog == QLatin1String("prolog"),
                               if (errorMessage)
                               *errorMessage = tr("Error while parsing first test result.");
                            return);
                    xmlWriter.writeStartElement(prolog.toString());
                    xmlWriter.writeAttributes(reader.attributes());
                    xmlWriter.writeEndElement();
                    xmlWriter.writeStartElement(QLatin1String("test"));
                    xmlWriter.writeAttributes(testAttributes);
                    isFirstTest = false;
                } else if (tagName == QLatin1String("epilog")) {
                    lastEpilogTime
                            = reader.attributes().value(QLatin1String("time")).toString();
                }
                xmlWriter.writeCurrentToken(reader);
                break;
            }
            case QXmlStreamReader::EndElement:
                if (reader.name() != QLatin1String("SquishReport"))
                    xmlWriter.writeCurrentToken(reader);
                break;
            case QXmlStreamReader::Characters:
                xmlWriter.writeCurrentToken(reader);
                break;
            // ignore the rest
            default:
                break;
            }
        }
        currentResultsFile.close();
    }
    if (!lastEpilogTime.isEmpty()) {
        xmlWriter.writeStartElement(QLatin1String("epilog"));
        xmlWriter.writeAttribute(QLatin1String("time"), lastEpilogTime);
        xmlWriter.writeEndElement();
    }
    xmlWriter.writeEndDocument();
}

Result::Type resultFromString(const QString &type)
{
    if (type == QLatin1String("LOG"))
        return Result::SQUISH_LOG;
    if (type == QLatin1String("PASS"))
        return Result::SQUISH_PASS;
    if (type == QLatin1String("FAIL"))
        return Result::SQUISH_FAIL;
    if (type == QLatin1String("WARNING"))
        return Result::SQUISH_WARN;
    if (type == QLatin1String("XFAIL"))
        return Result::SQUISH_EXPECTED_FAIL;
    if (type == QLatin1String("XPASS"))
        return Result::UNEXPECTED_PASS;
    if (type == QLatin1String("FATAL"))
        return Result::SQUISH_FATAL;
    if (type == QLatin1String("ERROR"))
        return Result::SQUISH_ERROR;
    return Result::SQUISH_LOG;
}

// this method uses the XML reader to parse output of the Squish results.xml and put it into an
// item that can be used to display inside the test results pane
void SquishXmlOutputHandler::outputAvailable(const QByteArray &output)
{
    static QString name;
    static QString details;
    static QString logDetails;
    static QString time;
    static QString file;
    static Result::Type type;
    static int line = 0;
    static bool prepend = false;

    m_xmlReader.addData(output);

    while (!m_xmlReader.atEnd()) {
        QXmlStreamReader::TokenType tokenType = m_xmlReader.readNext();
        switch (tokenType) {
        case QXmlStreamReader::StartDocument:
        case QXmlStreamReader::EndDocument:
            break;
        case QXmlStreamReader::StartElement: {
            const QString currentName = m_xmlReader.name().toString();
            // tags verification, message, epilog and test will start a new entry, so, reset values
            if (currentName == QLatin1String("verification")
                    || currentName == QLatin1String("message")
                    || currentName == QLatin1String("epilog")
                    || currentName == QLatin1String("test")) {
                name = currentName;
                details.clear();
                logDetails.clear();
                time.clear();
                file.clear();
                line = 0;
                type = Result::SQUISH_LOG;
            } else if (currentName == QLatin1String("result")) {
                // result tag won't add another entry, but gives more information on enclosing tag
                name = currentName;
            }

            // description tag could provide information that must be prepended to the former entry
            if (currentName == QLatin1String("description")) {
                prepend = (name == QLatin1String("result") && m_xmlReader.attributes().isEmpty());
            } else {
                foreach (const QXmlStreamAttribute &att, m_xmlReader.attributes()) {
                    const QString attributeName = att.name().toString();
                    if (attributeName == QLatin1String("time"))
                        time = QDateTime::fromString(att.value().toString(), Qt::ISODate)
                                .toString(QLatin1String("MMM dd, yyyy h:mm:ss AP"));
                    else if (attributeName == QLatin1String("file"))
                        file = att.value().toString();
                    else if (attributeName == QLatin1String("line"))
                        line = att.value().toInt();
                    else if (attributeName == QLatin1String("type"))
                        type = resultFromString(att.value().toString());
                    else if (attributeName == QLatin1String("name"))
                        logDetails = att.value().toString();
                }
            }
            // handle prolog (test) elements already within the start tag
            if (currentName == QLatin1String("prolog")) {
                TestResult result(QString(), QString(), QString(), Result::SQUISH_START,
                                  logDetails + QLatin1Char('\n') + time);
                result.setFileName(file);
                result.setLine(line);
                emit testResultCreated(result);
            }
            break;
        }
        case QXmlStreamReader::EndElement: {
            const QString currentName = m_xmlReader.name().toString();
            // description and result tags are handled differently, test tags are handled by
            // prolog tag (which is handled in QXmlStreamReader::StartElement already),
            // SquishReport tags will be ignored completely
            if (currentName == QLatin1String("epilog")) {
                TestResult result(QString(), QString(), QString(), Result::SQUISH_END, time);
                emit testResultCreated(result);
            } else if (currentName != QLatin1String("description")
                       && currentName != QLatin1String("prolog")
                       && currentName != QLatin1String("test")
                       && currentName != QLatin1String("result")
                       && currentName != QLatin1String("SquishReport")) {
                QString description;
                if (!logDetails.isEmpty())
                    description = logDetails + QLatin1Char('\n');

                TestResult result(QString(), QString(), QString(), type);
                result.setDescription(description + details.trimmed() + QLatin1Char('\n') + time);
                result.setFileName(file);
                result.setLine(line);
                emit testResultCreated(result);
            }
            break;
        }
        case QXmlStreamReader::Characters: {
            QStringRef text = m_xmlReader.text();
            if (m_xmlReader.isCDATA() || !text.trimmed().isEmpty()) {
                if (!m_xmlReader.isCDATA())
                    text = text.trimmed();
                if (prepend) {
                    if (!logDetails.isEmpty() && (text == QLatin1String("Verified")
                                                  || text == QLatin1String("Not Verified"))) {
                            logDetails.prepend(text + QLatin1String(": "));
                    } else {
                        logDetails = text.toString();
                    }
                } else {
                    details.append(text).append(QLatin1Char('\n'));
                }
            }
            break;
        }
        default:
            break;
        }
    }

    if (m_xmlReader.hasError()) {
        // kind of expected as we get the output piece by piece
        if (m_xmlReader.error() != QXmlStreamReader::PrematureEndOfDocumentError)
            qWarning() << m_xmlReader.error() << m_xmlReader.errorString();
    }
}

} // namespace Internal
} // namespace Autotest
