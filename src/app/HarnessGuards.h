// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_APP_HARNESSGUARDS_H_
#define SCANTAILOR_APP_HARNESSGUARDS_H_

// Checks shared by the three offscreen and live harnesses (ui_snapshot, retina_check, mac_check). Kept in a header
// with Qt Core only, so a test can call them without starting a harness.

#include <QDir>
#include <QFileInfo>
#include <QString>
#include <QStringList>

namespace harness {

namespace detail {

inline bool refuse(QString* error, const QString& text) {
  if (error != nullptr) {
    *error = text;
  }
  return false;
}

}  // namespace detail

// Accepts the output directory only if it lies strictly inside the root (the root itself is refused) and stores the
// resolved path in *resolvedOut. A lexical prefix check is not enough: a symlink inside the root would redirect what
// the harness writes and what retina_check later removes recursively, and the root itself must not be a valid
// output directory. The part of the path that exists is resolved through symlinks; the part that does not exist yet
// may not contain "..", so it cannot climb out after the check. Nothing is created here: the caller creates the
// directories only after this returns true.
inline bool outputDirIsStrictlyInside(const QString& outArg,
                                      const QString& root,
                                      QString* resolvedOut,
                                      QString* error) {
  const QString canonicalRoot = QDir(root).canonicalPath();
  if (canonicalRoot.isEmpty()) {
    return detail::refuse(error, QStringLiteral("the root directory %1 does not exist").arg(root));
  }
  if (outArg.isEmpty()) {
    return detail::refuse(error, QStringLiteral("the output directory is empty"));
  }

  // Absolute without cleaning, so ".." components survive until they are examined below.
  const QString absolute = QDir::isAbsolutePath(outArg) ? outArg : QDir::current().absolutePath() + "/" + outArg;
  QStringList components;
  for (const QString& component : absolute.split('/', Qt::SkipEmptyParts)) {
    if (component != ".") {
      components.append(component);
    }
  }

  // The deepest ancestor that exists; a dangling symlink counts as existing so that it is refused below.
  int existing = components.size();
  while (existing > 0) {
    const QFileInfo info("/" + components.mid(0, existing).join('/'));
    if (info.exists() || info.isSymLink()) {
      break;
    }
    --existing;
  }
  const QStringList tail = components.mid(existing);
  if (tail.contains("..")) {
    return detail::refuse(error, QStringLiteral("%1 has \"..\" below a directory that does not exist yet").arg(outArg));
  }

  const QString base = "/" + components.mid(0, existing).join('/');
  const QString canonicalBase = QFileInfo(base).canonicalFilePath();
  if (canonicalBase.isEmpty()) {
    return detail::refuse(error, QStringLiteral("cannot resolve %1").arg(base));
  }
  const QString resolved = tail.isEmpty() ? canonicalBase : QDir::cleanPath(canonicalBase + "/" + tail.join('/'));
  if (!resolved.startsWith(canonicalRoot + "/")) {
    return detail::refuse(error, QStringLiteral("%1 is not strictly inside %2").arg(resolved, canonicalRoot));
  }
  if (resolvedOut != nullptr) {
    *resolvedOut = resolved;
  }
  return true;
}

// Command line of ui_snapshot: one output directory and the optional flag --assert-no-language.
struct SnapshotArgs {
  QString outArg;
  bool assertNoLanguage = false;
  bool ok = false;
  QString error;
};

// args is the argument list without the program name. Every option other than --assert-no-language is refused, so a
// misspelt option cannot become the output directory.
inline SnapshotArgs parseSnapshotArgs(const QStringList& args) {
  SnapshotArgs result;
  int positionalCount = 0;
  for (const QString& arg : args) {
    if (arg == "--assert-no-language") {
      result.assertNoLanguage = true;
    } else if (arg.startsWith('-')) {
      result.error = QStringLiteral("unknown option %1").arg(arg);
      return result;
    } else {
      result.outArg = arg;
      ++positionalCount;
    }
  }
  if (positionalCount != 1) {
    result.error = QStringLiteral("expected exactly one output directory");
    return result;
  }
  result.ok = true;
  return result;
}

}  // namespace harness

#endif
