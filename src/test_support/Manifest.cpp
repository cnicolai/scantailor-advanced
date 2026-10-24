// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "Manifest.h"

#include <png.h>
#include <tiffio.h>

#include <QProcess>
#include <QRegularExpression>
#include <QSysInfo>
#include <QtGlobal>
#include <boost/version.hpp>
#include <cstdio>  // jpeglib.h needs FILE
#include <set>

#include <jpeglib.h>

// Compile definitions of the test_support target (see its CMakeLists.txt); the fallbacks only matter when a
// build system other than CMake compiles this file.
#ifndef ST_CMAKE_VERSION
#define ST_CMAKE_VERSION "unknown"
#endif
#ifndef ST_BUILD_TYPE
#define ST_BUILD_TYPE ""
#endif
#ifndef ST_XCODE_VERSION
#define ST_XCODE_VERSION "unknown"
#endif
#ifndef ST_QT_COMPILE_VERSION
#define ST_QT_COMPILE_VERSION "unknown"
#endif
#ifndef ST_FP_CONTRACT
#define ST_FP_CONTRACT "unset"
#endif
#ifndef ST_SOURCE_DIR
#define ST_SOURCE_DIR "."
#endif

#define ST_STRINGIFY_VALUE(x) #x
#define ST_STRINGIFY(x) ST_STRINGIFY_VALUE(x)

namespace test_support {

namespace {

const int kGitTimeoutMs = 30000;

QString libjpegVersion() {
#ifdef LIBJPEG_TURBO_VERSION
  // jconfig.h of libjpeg-turbo defines LIBJPEG_TURBO_VERSION as a bare token (3.2.0), hence the two-level macro.
  return QString::fromLatin1(ST_STRINGIFY(LIBJPEG_TURBO_VERSION));
#else
  // Plain libjpeg has no such macro; record the API version it reports instead.
  return QString::fromLatin1("JPEG_LIB_VERSION " ST_STRINGIFY(JPEG_LIB_VERSION));
#endif
}

QString compilerVersion() {
#ifdef __clang_version__
  return QString::fromLatin1(__clang_version__);
#else
  return QString::fromLatin1("unknown");
#endif
}

QString nonEmpty(const QString& value) {
  return value.isEmpty() ? QString::fromLatin1("unknown") : value;
}

}  // namespace

TextValues currentToolchain() {
  TextValues values;
  values["qt_runtime"] = nonEmpty(QString::fromLatin1(qVersion()));
  values["qt_compile"] = nonEmpty(QString::fromLatin1(ST_QT_COMPILE_VERSION));
  values["boost"] = nonEmpty(QString::fromLatin1(BOOST_LIB_VERSION));
  values["libtiff"] = nonEmpty(QString::fromLatin1(TIFFGetVersion()).section('\n', 0, 0).trimmed());
  values["libpng"] = nonEmpty(QString::fromLatin1(png_get_libpng_ver(nullptr)));
  values["libjpeg_turbo"] = nonEmpty(libjpegVersion());
  values["apple_clang"] = nonEmpty(compilerVersion());
  values["xcode"] = nonEmpty(QString::fromLatin1(ST_XCODE_VERSION));
  values["cmake"] = nonEmpty(QString::fromLatin1(ST_CMAKE_VERSION));
  values["macos"] = nonEmpty(QSysInfo::productVersion());
  values["ffp_contract"] = nonEmpty(QString::fromLatin1(ST_FP_CONTRACT));
  values["build_type"] = nonEmpty(QString::fromLatin1(ST_BUILD_TYPE));
  return values;
}

QString currentSourceCommit() {
  const QString unknown = QString::fromLatin1("unknown");

  QProcess git;
  git.setWorkingDirectory(QString::fromUtf8(ST_SOURCE_DIR));
  git.start("git", QStringList() << "rev-parse" << "HEAD");
  if (!git.waitForStarted(kGitTimeoutMs)) {
    return unknown;
  }
  if (!git.waitForFinished(kGitTimeoutMs)) {
    git.kill();
    git.waitForFinished();
    return unknown;
  }
  if (git.exitStatus() != QProcess::NormalExit || git.exitCode() != 0) {
    return unknown;
  }

  const QString commit = QString::fromLatin1(git.readAllStandardOutput()).trimmed();
  const QRegularExpression hash(QRegularExpression::anchoredPattern("[0-9a-f]{40}"));
  return hash.match(commit).hasMatch() ? commit : unknown;
}

QStringList toolchainDifferences(const TextValues& recorded, const TextValues& current) {
  std::set<QString> keys;
  for (const auto& entry : recorded) {
    keys.insert(entry.first);
  }
  for (const auto& entry : current) {
    keys.insert(entry.first);
  }

  const QString missing = QString::fromLatin1("<missing>");
  QStringList differences;
  for (const QString& key : keys) {
    if (key == "source_commit" || key == "build_type") {
      continue;
    }
    const auto r = recorded.find(key);
    const auto c = current.find(key);
    const QString recordedValue = r == recorded.end() ? missing : r->second;
    const QString currentValue = c == current.end() ? missing : c->second;
    if (recordedValue != currentValue) {
      differences << QString("%1: recorded '%2', now '%3'").arg(key, recordedValue, currentValue);
    }
  }
  return differences;
}

}  // namespace test_support
