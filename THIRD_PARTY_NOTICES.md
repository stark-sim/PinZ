# Third-Party Notices

EShot includes or can distribute the following third-party components.

## Tesseract OCR

- Project: Tesseract OCR
- Website: https://github.com/tesseract-ocr/tesseract
- License: Apache License 2.0

Tesseract OCR is used for optical character recognition when the OCR feature is installed and enabled.

## Tesseract Language Data

- Project: tessdata
- Website: https://github.com/tesseract-ocr/tessdata
- License: Apache License 2.0

EShot may distribute selected Tesseract traineddata files for OCR language support.

## FFmpeg

- Project: FFmpeg
- Website: https://ffmpeg.org/
- License: FFmpeg is available under LGPL/GPL license terms depending on the distributed build.

EShot uses FFmpeg for MP4 screen recording when the optional video recording component is installed. The bundled FFmpeg binary should be distributed with its corresponding license information from the selected FFmpeg build.

## GStreamer and PipeWire

- Project: GStreamer
- Website: https://gstreamer.freedesktop.org/
- License: GStreamer core and base libraries are LGPL-2.1-or-later; plugin licenses vary.

Linux fast capture links the system GStreamer core, app, and video libraries and uses the system PipeWire source plugin. Distributions that bundle these libraries or plugins must include their corresponding license and source-offer notices.

## Qt

- Project: Qt
- Website: https://www.qt.io/
- License: Qt is available under open source and commercial licenses. EShot is built with Qt and distributes the runtime files required by the application.

## Inno Setup

- Project: Inno Setup
- Website: https://jrsoftware.org/isinfo.php
- License: Inno Setup License

Inno Setup is used to build the Windows installer.

## Notes

This file is provided for attribution and license visibility. EShot's own source code is licensed under the MIT License; see LICENSE.
