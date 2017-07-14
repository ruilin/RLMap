/*******************************************************************************
 The MIT License (MIT)

 Copyright (c) 2014 Alexander Zolotarev <me@alex.bio> from Minsk, Belarus

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 *******************************************************************************/

package org.alohalytics.test;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;

public class Util {

  public static String ReadFileAsUtf8String(String filePath) throws IOException {
    final File file = new File(filePath);
    final long fileLength = file.length();
    if (fileLength > Integer.MAX_VALUE) {
      throw new IOException(filePath + " size is too large: " + fileLength);
    }
    final byte[] buffer = new byte[(int) fileLength];
    final FileInputStream istream = new FileInputStream(file);
    try {
      if (fileLength != istream.read(buffer)) {
        throw new IOException("Error while reading contents of " + filePath);
      }
    } finally {
      istream.close();
    }
    return new String(buffer, "UTF-8");
  }

  public static void WriteStringToFile(String toWrite, String filePath) throws IOException {
    final FileOutputStream ostream = new FileOutputStream(filePath);
    try {
      ostream.write(toWrite.getBytes());
    } finally {
      ostream.close();
    }
  }
}
