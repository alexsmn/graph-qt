#include <gmock/gmock.h>

#include <QApplication>

// From build-support/, put on the include path by the product root; see the
// header for why it lives there and not beside this file.
#include "scada_qt_offscreen_platform.h"

int main(int argc, char** argv) {
  // Before QApplication, which reads QT_QPA_PLATFORM in its constructor.
  scada::qt_test::DefaultToOffscreenPlatform();

  QApplication app{argc, argv};
  ::testing::InitGoogleMock(&argc, argv);
  return RUN_ALL_TESTS();
}
