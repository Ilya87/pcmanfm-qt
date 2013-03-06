/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2013  PCMan <email>

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


#include "utilities.h"
#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QUrl>
#include <QList>
#include <QStringBuilder>
#include <QInputDialog>
#include <QMessageBox>
#include "fileoperation.h"

using namespace Fm;

namespace Fm {

FmPathList* pathListFromQUrls(QList<QUrl> urls) {
  QList<QUrl>::const_iterator it;
  FmPathList* pathList = fm_path_list_new();
  for(it = urls.begin(); it != urls.end(); ++it) {
    QUrl url = *it;
    FmPath* path = fm_path_new_for_uri(url.toString().toUtf8());
    fm_path_list_push_tail(pathList, path);
    fm_path_unref(path);
  }
  return pathList;
}

void pasteFilesFromClipboard(FmPath* destPath, QWidget* parent) {
  QClipboard* clipboard = QApplication::clipboard();
  const QMimeData* data = clipboard->mimeData();
  bool isCut = false;
  FmPathList* paths = NULL;
  if(data->hasFormat("x-special/gnome-copied-files")) {
    // Gnome, LXDE, and XFCE
    QByteArray gnomeData = data->data("x-special/gnome-copied-files");
    char* pdata = gnomeData.data();
    char* eol = strchr(pdata, '\n');
    if(eol) {
      *eol = '\0';
      isCut = (strcmp(pdata, "cut") == 0 ? true : false);
      paths = fm_path_list_new_from_uri_list(eol + 1);
    }
  }
  
  if(!paths && data->hasUrls()) {
    // The KDE way
    paths = Fm::pathListFromQUrls(data->urls());
    QByteArray cut = data->data("x-kde-cut-selection");
    if(!cut.isEmpty() && cut.at(0) == '1')
      isCut = true;
  }
  if(paths) {
    if(isCut)
      FileOperation::moveFiles(paths, destPath, parent);
    else
      FileOperation::copyFiles(paths, destPath, parent);
    fm_path_list_unref(paths);
  }
}

void copyFilesToClipboard(FmPathList* files) {
  QClipboard* clipboard = QApplication::clipboard();
  QMimeData* data = new QMimeData();
  char* urilist = fm_path_list_to_uri_list(files);
  // Gnome, LXDE, and XFCE
  data->setData("x-special/gnome-copied-files", (QString("copy\n") + urilist).toUtf8());
  // The KDE way
  data->setData("text/uri-list", urilist);
  // data.setData("x-kde-cut-selection", "0");
  g_free(urilist);
  clipboard->setMimeData(data);
}

void cutFilesToClipboard(FmPathList* files) {
  QClipboard* clipboard = QApplication::clipboard();
  QMimeData* data = new QMimeData();
  char* urilist = fm_path_list_to_uri_list(files);
  // Gnome, LXDE, and XFCE
  data->setData("x-special/gnome-copied-files", (QString("cut\n") + urilist).toUtf8());
  // The KDE way
  data->setData("text/uri-list", urilist);
  data->setData("x-kde-cut-selection", "1");
  g_free(urilist);
  clipboard->setMimeData(data);
}

void renameFile(FmPath* file, QWidget* parent) {
  GFile *gf, *parent_gf, *dest;
  GError* err = NULL;
  QString new_name;
  bool ok;
  new_name = QInputDialog::getText(parent, QObject::tr("Rename File"),
                                   QObject::tr("Please enter a new name:"),
                                   QLineEdit::Normal,
                                   fm_path_get_basename(file),
                                   &ok);
  if(!ok)
    return;
  gf = fm_path_to_gfile(file);
  parent_gf = g_file_get_parent(gf);
  dest = g_file_get_child(G_FILE(parent_gf), new_name.toLocal8Bit().data());
  g_object_unref(parent_gf);
  if(!g_file_move(gf, dest,
    G_FILE_COPY_ALL_METADATA|
    G_FILE_COPY_NO_FALLBACK_FOR_MOVE|
    G_FILE_COPY_NOFOLLOW_SYMLINKS,
    NULL, /* make this cancellable later. */
    NULL, NULL, &err))
  {
    QMessageBox::critical(parent, QObject::tr("Error"), err->message);
    g_error_free(err);
  }
  g_object_unref(dest);
  g_object_unref(gf);
}

};
