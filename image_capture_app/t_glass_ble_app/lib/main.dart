import 'dart:async';
import 'dart:io';
import 'dart:typed_data';
import 'package:image/image.dart' as img;
import 'package:shared_preferences/shared_preferences.dart';
import 'package:flutter/foundation.dart';
import 'package:bitsdojo_window/bitsdojo_window.dart';
import 'package:flutter/material.dart';
import 'package:path_provider/path_provider.dart';
import 'package:screen_capturer/screen_capturer.dart';
import 'package:system_tray/system_tray.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';

void main() async {
  WidgetsFlutterBinding.ensureInitialized();
  runApp(
    const MyApp(),
  );

  doWhenWindowReady(() {
    final win = appWindow;
    const initialSize = Size(600, 450);
    win.minSize = initialSize;
    win.size = initialSize;
    win.alignment = Alignment.center;
    win.title = "T-Glass BLE APP";
    win.show();
  });
}

String getTrayImagePath(String imageName) {
  return Platform.isWindows ? 'assets/$imageName.ico' : 'assets/$imageName.png';
}

String getImagePath(String imageName) {
  return Platform.isWindows ? 'assets/$imageName.bmp' : 'assets/$imageName.png';
}

class MyApp extends StatefulWidget {
  const MyApp({Key? key}) : super(key: key);

  @override
  State<MyApp> createState() => _MyAppState();
}

class _MyAppState extends State<MyApp> {
  //SCREENSHOT
  Directory? _screenshotsDirectory;
  CapturedData? _lastCapturedData;

  //BLE
  bool _isScanning = false;
  bool _isConnected = false;
  String _buttonText = "Start Scanning";

  final String ServiceUUID = "00ff";
  final String charUUID = "ff01";
  final int desiredMtu = 215;   // [[[You need to check the set MTU size.]]]

  String _targetDeviceName = ""; // Target device name entered by user

  final AppWindow _appWindow = AppWindow();
  final SystemTray _systemTray = SystemTray();
  final Menu _trayMenu = Menu();

  bool _isDarkMode = false;

  BluetoothDevice? _connectedDevice;
  BluetoothCharacteristic? _targetCharacteristic;

  /// Load saved theme preference
  Future<void> _loadThemePreference() async {
    SharedPreferences prefs = await SharedPreferences.getInstance();
    setState(() {
      _isDarkMode = prefs.getBool('isDarkMode') ?? false;
    });
  }

  /// Toggle theme and save preference
  Future<void> _toggleTheme() async {
    setState(() {
      _isDarkMode = !_isDarkMode;
    });
    SharedPreferences prefs = await SharedPreferences.getInstance();
    await prefs.setBool('isDarkMode', _isDarkMode);
  }

  Future<void> _handleClickCapture(CaptureMode mode) async {
    Directory directory =
        _screenshotsDirectory ?? await getApplicationDocumentsDirectory();

    String imageName =
        'Screenshot-${DateTime.now().millisecondsSinceEpoch}.png';
    String imagePath =
        '${directory.path}/screen_capturer_example/Screenshots/$imageName';
    _lastCapturedData = await screenCapturer.capture(
      mode: mode,
      imagePath: imagePath,
      copyToClipboard: false,
      silent: true,
    );
    if (_lastCapturedData != null) {
      // ignore: avoid_print
      // print(_lastCapturedData!.toJson());
    } else {
      // ignore: avoid_print
      print('User canceled capture');
    }
    setState(() {});
  }

  @override
  void initState() {
    super.initState();
    _loadThemePreference();
    _initSystemTray();
  }

  @override
  void dispose() {
    super.dispose();
  }

  Future<Uint8List> convertPngToRgb565(String imagePath) async {
    // Load the image file
    File file = File(imagePath);
    Uint8List imageBytes = await file.readAsBytes();

    // Decode the PNG image
    img.Image? image = img.decodeImage(imageBytes);
    if (image == null) {
      throw Exception("Failed to decode image.");
    }

    // Define target size
    const int targetSize = 126;

    // Resize while maintaining aspect ratio
    img.Image resizedImage = img.copyResize(image,
        width: targetSize, height: targetSize, maintainAspect: true);

    // Create a blank 126x126 image with a black background (or any color you want)
    Uint8List blankImageBytes =
        Uint8List(targetSize * targetSize * 4); // 4 bytes per pixel (RGBA)
    ByteBuffer buffer =
        Uint8List.fromList(blankImageBytes).buffer; // Convert to ByteBuffer

    // Create a blank 126x126 image with a black background (or any color you want)
    img.Image finalImage = img.Image.fromBytes(
        width: targetSize, height: targetSize, bytes: buffer);

    // Draw the resized image centered onto the final canvas
    int offsetX = (targetSize - resizedImage.width) ~/ 2;
    int offsetY = (targetSize - resizedImage.height) ~/ 2;
    img.compositeImage(finalImage, resizedImage, dstX: offsetX, dstY: offsetY);

    // Prepare RGB565 buffer
    Uint8List rgb565Data = Uint8List(targetSize * targetSize * 2);
    ByteData byteData = ByteData.view(rgb565Data.buffer);

    int index = 0;
    for (int y = 0; y < finalImage.height; y++) {
      for (int x = 0; x < finalImage.width; x++) {
        img.Pixel pixel = finalImage.getPixel(x, y);

        // Extract RGB components and cast to int
        int r = pixel.r.toInt();
        int g = pixel.g.toInt();
        int b = pixel.b.toInt();

        // Convert to RGB565 format
        int rgb565 = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);

        // Store as two bytes (Little Endian format)
        byteData.setUint16(index, rgb565, Endian.little);
        index += 2;
      }
    }

    return rgb565Data;
  }

  /// Function to send data chunk over BLE
  Future<void> _sendChunkToBLE(Uint8List chunk) async {
    // Replace with your actual BLE characteristic write function

    if (_targetCharacteristic != null) {
      try {
        await _targetCharacteristic!.write(chunk,
            withoutResponse: true, allowLongWrite: false, timeout: 60);
        debugPrint("Data written to characteristic.");
      } catch (e) {
        debugPrint("Failed to write data: $e");
      }
    }
  }

  void _sendImageOverBLE() async {
    if (_lastCapturedData == null || _lastCapturedData!.imagePath == null) {
      print("No image to send.");
      return;
    }

    try {
      Uint8List rgb565Data =
          await convertPngToRgb565(_lastCapturedData!.imagePath!);

      // Send over BLE or use for display
      print("RGB565 Data Length: ${rgb565Data.length}");

      // Send in chunks (assuming BLE characteristic write method)
      int chunkSize = desiredMtu; // Adjust based on BLE MTU size
      for (int i = 0; i < rgb565Data.length; i += chunkSize) {
        int end = (i + chunkSize < rgb565Data.length)
            ? i + chunkSize
            : rgb565Data.length;
        Uint8List chunk = rgb565Data.sublist(i, end);

        // Send the chunk over BLE (example method, replace with your actual BLE library)
        await _sendChunkToBLE(chunk);
      }
    } catch (e) {
      print("Error: $e");
    }

    print("Image sent over BLE successfully!");
  }

  void _toggleScanOrDisconnect() async {
    if (_isConnected) {
      // Disconnect
      await _connectedDevice?.disconnect();
      debugPrint("Disconnected from device.");

      // Wait for clean disconnection
      await Future.delayed(const Duration(seconds: 1));

      setState(() {
        _isConnected = false;
        _buttonText = "Start Scanning";
      });
    } else {
      if (_isScanning) {
        await FlutterBluePlus.stopScan();
        debugPrint("Scanning stopped.");
        setState(() {
          _isScanning = false;
          _buttonText = "Start Scanning";
        });
      } else {
        if (_targetDeviceName.isEmpty) {
          debugPrint("Please enter a device name.");
          return;
        }

        // Ensure Bluetooth is ON before scanning
        await FlutterBluePlus.adapterState
            .firstWhere((state) => state == BluetoothAdapterState.on);

        await FlutterBluePlus.stopScan(); // Stop any previous scan
        debugPrint("Reset BLE state.");

        // Start scanning
        FlutterBluePlus.startScan(timeout: const Duration(seconds: 10));
        setState(() {
          _isScanning = true;
          _buttonText = "Stop Scanning";
        });

        // Listen for scan results
        FlutterBluePlus.scanResults.listen((results) async {
          for (ScanResult r in results) {
            if (r.device.advName == _targetDeviceName) {
              debugPrint("Found target device: $_targetDeviceName");

              // Stop scanning
              await FlutterBluePlus.stopScan();
              setState(() {
                _isScanning = false;
                _buttonText = "Connecting...";
              });

              // Connect to the device
              try {
                await r.device.connect();
                _connectedDevice = r.device;
                debugPrint("Connected to $_targetDeviceName");

                setState(() {
                  _isConnected = true;
                  _buttonText = "Disconnect";
                });

                // Discover services and characteristics
                List<BluetoothService> services =
                    await r.device.discoverServices();
                for (BluetoothService service in services) {
                  for (BluetoothCharacteristic characteristic
                      in service.characteristics) {
                    // Find the characteristic with the target UUID
                    if (characteristic.uuid.toString() == charUUID) {
                      _targetCharacteristic = characteristic;
                      debugPrint(
                          "Found target characteristic: ${characteristic.uuid.toString()}");
                    }
                  }
                }
              } catch (e) {
                debugPrint("Failed to connect: $e");
                setState(() {
                  _buttonText = "Start Scanning";
                });
              }
              return; // Exit after connecting
            }
          }
        });
      }
    }
  }

  Future<void> _initSystemTray() async {
    // We first init the systray menu and then add the menu entries
    await _systemTray.initSystemTray(iconPath: getTrayImagePath('app_icon'));
    _systemTray.setTitle("T-Glass");
    _systemTray.setToolTip("T-Glass BT App");

    // handle system tray event
    _systemTray.registerSystemTrayEventHandler((eventName) {
      debugPrint("eventName: $eventName");
      if (eventName == kSystemTrayEventClick) {
        Platform.isWindows ? _appWindow.show() : _systemTray.popUpContextMenu();
      } else if (eventName == kSystemTrayEventRightClick) {
        Platform.isWindows ? _systemTray.popUpContextMenu() : _appWindow.show();
      }
    });

    await _trayMenu.buildFrom([
      MenuItemLabel(
        label: 'Capture & Send',
        image: getImagePath('app_icon'),
        onClicked: (menuItem) async {
          if (_isConnected) {
            await _handleClickCapture(CaptureMode.region);
            _sendImageOverBLE();
          }
        },
      ),
      MenuSeparator(),
      MenuItemLabel(label: 'Show', onClicked: (menuItem) => _appWindow.show()),
      MenuItemLabel(label: 'Hide', onClicked: (menuItem) => _appWindow.hide()),
      MenuItemLabel(
        label: 'Exit',
        onClicked: (menuItem) => _appWindow.close(),
      ),
    ]);

    _systemTray.setContextMenu(_trayMenu);
  }

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      debugShowCheckedModeBanner: false,
      theme: _isDarkMode ? ThemeData.dark() : ThemeData.light(),
      home: Scaffold(
        body: WindowBorder(
          color: const Color(0xFF805306),
          width: 1,
          child: Column(
            children: [
              const TitleBar(
                appName: 'Capture Tool for T-Glass',
              ),
              Align(
                alignment: Alignment.topRight,
                child: IconButton(
                  icon: Icon(_isDarkMode ? Icons.light_mode : Icons.dark_mode),
                  onPressed: _toggleTheme,
                  tooltip: "Toggle Dark Mode",
                ),
              ),
              Expanded(
                child: Padding(
                  padding: const EdgeInsets.all(20.0),
                  child: Column(
                    mainAxisAlignment: MainAxisAlignment.center,
                    children: [
                      Row(
                        crossAxisAlignment: CrossAxisAlignment.center,
                        children: [
                          Expanded(
                            child: TextField(
                              decoration: const InputDecoration(
                                labelText: "Enter Target Device Name",
                                border: OutlineInputBorder(),
                              ),
                              onChanged: (value) {
                                setState(() {
                                  _targetDeviceName = value;
                                });
                              },
                              readOnly: _isConnected,
                            ),
                          ),
                          const SizedBox(width: 10),
                          ElevatedButton(
                            style: ElevatedButton.styleFrom(
                              shape: RoundedRectangleBorder(
                                borderRadius: BorderRadius.circular(8),
                              ),
                              padding: const EdgeInsets.symmetric(
                                  horizontal: 16, vertical: 14),
                              textStyle: const TextStyle(
                                  fontSize: 16, fontWeight: FontWeight.bold),
                            ),
                            onPressed: _toggleScanOrDisconnect,
                            child: Text(_buttonText),
                          ),
                        ],
                      ),
                      const SizedBox(height: 20),

                      // Image display section
                      if (_isConnected &&
                          _lastCapturedData != null &&
                          _lastCapturedData?.imagePath != null)
                        Expanded(
                          child: Container(
                            width: double.infinity,
                            decoration: BoxDecoration(
                              border: Border.all(color: Colors.grey, width: 2),
                              borderRadius: BorderRadius.circular(8),
                            ),
                            child: ClipRRect(
                              borderRadius: BorderRadius.circular(6),
                              child: Image.file(
                                File(_lastCapturedData!.imagePath!),
                                fit: BoxFit.contain, // Prevent overflow
                              ),
                            ),
                          ),
                        ),

                      if (_isConnected &&
                          _lastCapturedData != null &&
                          _lastCapturedData?.imagePath != null)
                        SizedBox(
                          width: double.infinity, // Full width button
                          child: FilledButton(
                            onPressed: () {
                              _sendImageOverBLE();
                            },
                            child: const Text('Send'),
                          ),
                        ),

                      if (_isConnected) const SizedBox(height: 20),
                      if (_isConnected)
                        SizedBox(
                          width: double.infinity, // Full width button
                          child: FilledButton(
                            onPressed: () {
                              _handleClickCapture(CaptureMode.region);
                            },
                            child: const Text('Capture Region'),
                          ),
                        ),
                    ],
                  ),
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }
}

class TitleBar extends StatelessWidget {
  const TitleBar({Key? key, required this.appName}) : super(key: key);
  final String appName;

  @override
  Widget build(BuildContext context) {
    return WindowTitleBarBox(
      child: Row(
        children: [
          const Spacer(),
          Padding(
            padding: const EdgeInsets.only(left: 8.0),
            child: Text(appName),
          ),
          Expanded(
            child: MoveWindow(),
          ),
          const WindowButtons()
        ],
      ),
    );
  }
}

final buttonColors = WindowButtonColors(
    iconNormal: const Color(0xFF805306),
    mouseOver: const Color(0xFFF6A00C),
    mouseDown: const Color(0xFF805306),
    iconMouseOver: const Color(0xFF805306),
    iconMouseDown: const Color(0xFFFFD500));

final closeButtonColors = WindowButtonColors(
    mouseOver: const Color(0xFFD32F2F),
    mouseDown: const Color(0xFFB71C1C),
    iconNormal: const Color(0xFF805306),
    iconMouseOver: Colors.white);

class WindowButtons extends StatelessWidget {
  const WindowButtons({Key? key}) : super(key: key);

  @override
  Widget build(BuildContext context) {
    return Row(
      children: [
        MinimizeWindowButton(colors: buttonColors),
        MaximizeWindowButton(colors: buttonColors),
        CloseWindowButton(
          colors: closeButtonColors,
          onPressed: () {
            showDialog<void>(
              context: context,
              barrierDismissible: false,
              builder: (BuildContext context) {
                return AlertDialog(
                  title: const Text('Exit Program?'),
                  content: const Text(
                      ('The window will be hidden, to exit the program you can use the system menu.')),
                  actions: <Widget>[
                    TextButton(
                      child: const Text('OK'),
                      onPressed: () {
                        Navigator.of(context).pop();
                        appWindow.hide();
                      },
                    ),
                  ],
                );
              },
            );
          },
        ),
      ],
    );
  }
}
