/*
    Copyright (C) 2009  Arno Rehn <arno@arnorehn.de>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "generatorpreprocessor.h"
#include "generatorenvironment.h"

#include <rpp/pp-environment.h>
#include <rpp/pp-macro.h>

#include <QtDebug>

Preprocessor::Preprocessor(const QList<QDir>& includeDirs, const QStringList& defines, const QFileInfo& file)
    : m_includeDirs(includeDirs), m_defines(defines), m_file(file)
{
    pp = new rpp::pp(this);
    pp->setEnvironment(new GeneratorEnvironment(pp));
    if (file.exists())
        m_fileStack.push(file);
    
    m_topBlock = new rpp::MacroBlock(0);
    
    // some basic definitions
    rpp::pp_macro* exportMacro = new rpp::pp_macro;
    exportMacro->name = IndexedString("__cplusplus");
    exportMacro->definition.append(IndexedString('1'));
    exportMacro->function_like = false;
    exportMacro->variadics = false;
    m_topBlock->setMacro(exportMacro);

    exportMacro = new rpp::pp_macro;
    exportMacro->name = IndexedString("__GNUC__");
    exportMacro->definition.append(IndexedString('4'));
    exportMacro->function_like = false;
    exportMacro->variadics = false;
    m_topBlock->setMacro(exportMacro);

    exportMacro = new rpp::pp_macro;
    exportMacro->name = IndexedString("__GNUC_MINOR__");
    exportMacro->definition.append(IndexedString('1'));
    exportMacro->function_like = false;
    exportMacro->variadics = false;
    m_topBlock->setMacro(exportMacro);

    exportMacro = new rpp::pp_macro;
    exportMacro->name = IndexedString("__linux__");
    exportMacro->function_like = false;
    exportMacro->variadics = false;
    m_topBlock->setMacro(exportMacro);
    
    foreach (QString define, defines) {
        exportMacro = new rpp::pp_macro;
        exportMacro->name = IndexedString(define);
        exportMacro->function_like = false;
        exportMacro->variadics = false;
        m_topBlock->setMacro(exportMacro);
    }
    
    pp->environment()->visitBlock(m_topBlock);
}


Preprocessor::~Preprocessor()
{
    delete pp;
}

void Preprocessor::setFile(const QFileInfo& file)
{
    m_fileStack.clear();
    m_fileStack.push(file);
    m_file = file;
}

QFileInfo Preprocessor::file()
{
    return m_file;
}


void Preprocessor::setIncludeDirs(QList< QDir > dirs)
{
    m_includeDirs = dirs;
}


QList< QDir > Preprocessor::includeDirs()
{
    return m_includeDirs;
}


void Preprocessor::setDefines(QStringList defines)
{
    m_defines = defines;
}


QStringList Preprocessor::defines()
{
    return m_defines;
}


PreprocessedContents Preprocessor::lastContents()
{
    return m_contents;
}


PreprocessedContents Preprocessor::preprocess()
{
    m_contents = pp->processFile(m_file.absoluteFilePath());
    return m_contents;
}

rpp::Stream* Preprocessor::sourceNeeded(QString& fileName, rpp::Preprocessor::IncludeType type, int sourceLine, bool skipCurrentPath)
{
    // are the contents already cached?
    if (type == rpp::Preprocessor::IncludeGlobal && m_cache.contains(fileName)) { 
        QPair<QFileInfo, PreprocessedContents>& cached = m_cache[fileName];
        m_fileStack.push(cached.first);
        return new HeaderStream(&cached.second, &m_fileStack);
    }
    
    QString path;
    if (type == rpp::Preprocessor::IncludeLocal) {
        if (m_fileStack.last().absoluteDir().exists(fileName))
            path = m_fileStack.last().absoluteDir().filePath(fileName);
    } else {
        foreach (QDir dir, m_includeDirs) {
            if (dir.exists(fileName)) {
                path = dir.filePath(fileName);
                break;
            }
        }
    }
    
    if (path.isEmpty())
        return 0;
    
    QFile file(path);
    file.open(QFile::ReadOnly);
    QByteArray array = file.readAll();
    file.close();
    
    if (type == rpp::Preprocessor::IncludeGlobal) {
        // cache the identifier (fileName), the accompanying QFileInfo and the contents
        QHash<QString, QPair<QFileInfo, PreprocessedContents> >::iterator iter =
            m_cache.insert(fileName, qMakePair(QFileInfo(path), convertFromByteArray(array)));
        /* Push the QFileInfo of the included file on the file stack, so if sourceNeeded() is called for that file,
           we know where to search for headers included with a IncludeLocal. The HeaderStream will be destroyed when the
           file has been processed and it'll then pop() the stack */
        m_fileStack.push(iter.value().first);
        return new HeaderStream(&iter.value().second, &m_fileStack);
    } else {
        /* Don't cache locally included files - the meaning of a file name may change when recursing into
           other directories.
           We also don't need to push any files on the fileStack because the directory for local includes is
           not changing. */
        m_localContent.append(convertFromByteArray(array));
        return new rpp::Stream(&m_localContent.last());
    }
    return 0;
}

