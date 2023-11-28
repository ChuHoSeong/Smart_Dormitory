///
/// main.dart
/// - 앱 초기화
/// - 기본 구조체
/// 

import 'dart:async';
import 'dart:math' as math;

import 'package:firebase_core/firebase_core.dart';
import 'package:firebase_database/firebase_database.dart';
import 'package:flutter/material.dart';
import 'package:flutter_native_splash/flutter_native_splash.dart';

import 'firebase_options.dart';

late final DatabaseReference DBref;
late final StreamController<String> sctr;

Future<void> main() async {
  WidgetsBinding widgetsBinding = WidgetsFlutterBinding.ensureInitialized();
  FlutterNativeSplash.preserve(widgetsBinding: widgetsBinding);
  await Firebase.initializeApp(options: DefaultFirebaseOptions.currentPlatform)
      .whenComplete(() => FlutterNativeSplash.remove());
  DBref = FirebaseDatabase.instance.ref();
  sctr = StreamController<String>.broadcast();
  runApp(MyApp());
}

class MyApp extends StatelessWidget {
  @override
  Widget build(BuildContext context) {
    return MaterialApp(
        debugShowCheckedModeBanner: false,
        title: 'Smart Dormitory',
        theme: ThemeData(
          appBarTheme: AppBarTheme(
            backgroundColor: Colors.transparent,
            elevation: 0,
            titleTextStyle: TextStyle(
              fontSize: 24,
              color: Colors.black,
              fontWeight: FontWeight.bold
            )
          ),
        ),
        home: Frame());
  }
}
class Frame extends StatelessWidget {
  const Frame({super.key});

  @override
  Widget build(BuildContext context) {
    return Stack(
        children: [
          Scaffold(
            appBar: AppBar(
              title: Text("Smart Dormitory"),
              actions: [
                Padding(
                  padding: EdgeInsets.only(right: 11),
                  child: Transform(
                    alignment: Alignment.center,
                    transform: Matrix4.rotationY(math.pi),
                    child: Icon(
                      Icons.message_rounded,
                      size: 32,
                      color: Colors.black,
                    ),
                  ),
                )
              ],
            ),
            body: Padding(
              padding: EdgeInsets.only(top: 40),
              child: Column(children: [
                Column(
                  mainAxisSize: MainAxisSize.min,
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Padding(
                      padding: EdgeInsets.only(left: 18),
                      child: Text("기숙사 상태", style: TextStyle(
                          fontSize: 18, fontWeight: FontWeight.bold)),
                    ),
                    SizedBox(height: 20),
                    SingleChildScrollView(
                      scrollDirection: Axis.horizontal,
                      child: Row(
                          children: [
                            DormitoryStatus(
                              margin: EdgeInsets.only(left: 24, right:  6, top: 10, bottom: 10),
                              name: "공기 상태",
                              path: "Air Cleaner/quality"
                            ),
                            DormitoryStatus(
                              margin: EdgeInsets.only(left:  6, right:  6),
                              name: "습도",
                              path: "Humidifier/humidity"
                            ),
                            DormitoryStatus(
                              margin: EdgeInsets.only(left:  6, right: 24),
                              name: "온도",
                              path: "Humidifier/temp"
                            )
                          ]
                      ),
                    )
                  ],
                ),
                SizedBox(height: 42),
                Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Padding(
                        padding: EdgeInsets.only(left: 18),
                        child: Text("연결된 디바이스", style: TextStyle(
                            fontSize: 18, fontWeight: FontWeight.bold)),
                      ),
                      Padding(
                          padding: EdgeInsets.fromLTRB(24, 20, 24, 0),
                          child: Column(children: [
                            Row(
                                mainAxisSize: MainAxisSize.max,
                                mainAxisAlignment: MainAxisAlignment.spaceBetween,
                                children: [
                                  DeviceStatus(
                                    w: 100,
                                    h: 100,
                                    name: "스위치1",
                                    path: "Switch/power/0",
                                  ),
                                  DeviceStatus(
                                    w: 100,
                                    h: 100,
                                    name: "스위치2",
                                    path: "Switch/power/1",
                                  ),
                                  DeviceStatus(
                                    w: 100,
                                    h: 100,
                                    name: "스위치3",
                                    path: "Switch/power/2",
                                  ),
                                ]
                            ),
                            SizedBox(height: 20),
                            Row(
                                mainAxisSize: MainAxisSize.max,
                                mainAxisAlignment: MainAxisAlignment.spaceBetween,
                                children: [
                                  DeviceStatus(
                                    w: 100,
                                    h: 100,
                                    name: "공기청정기",
                                    path: "Air Cleaner/power",
                                  ),
                                  DeviceStatus(
                                    w: 100,
                                    h: 100,
                                    name: "가습기",
                                    path: "Humidifier/power",
                                  ),
                                  SizedBox(width: 100)
                                ]
                            )
                          ])
                      )
                    ]
                )
              ])
            )
          ),
          Positioned(
            top: 70,
            right: 11,
            child: MessageBox()
          )
        ]
    );
  }
}

class DormitoryStatus extends StatefulWidget {
  const DormitoryStatus({super.key, required this.margin, required this.name, required this.path});

  final EdgeInsets margin;
  final String name;
  final String path;

  @override
  State<DormitoryStatus> createState() => _DormitoryStatusState();
}
class _DormitoryStatusState extends State<DormitoryStatus> {

  var data;
  late String std;
  late String imgPath;

  @override
  void initState() {
    switch (widget.name) {
      case "공기 상태":
        std = "㎛";
        imgPath = "assets/mask.png";
        break;
      case "습도":
        std = "%";
        imgPath = "assets/water.png";
        break;
      case "온도":
        std = "℃";
        imgPath = "assets/temp_mid.png";
        break;
      default:
        std = "";
        break;
    }
    super.initState();
  }

  @override
  Widget build(BuildContext context) {
    return StreamBuilder(
      stream: DBref.child(widget.path).onValue,
      builder: (context, snapshot) {
        if (snapshot.hasData) {
          data = snapshot.data!.snapshot.value;
          print("${widget.path}: $data");
        }
        return Container(
          width: 160,
          height: 190,
          margin: widget.margin,
          padding: EdgeInsets.symmetric(horizontal: 14, vertical: 8),
          decoration: BoxDecoration(
            color: Colors.white,
            borderRadius: BorderRadius.circular(20),
            boxShadow: [BoxShadow(
              color: Colors.grey.shade400,
              blurRadius: 2,
              spreadRadius: 2,
              offset: Offset(2, 2)
            )]
          ),
          alignment: Alignment.center,
          child: data == null
              ? Center(child: CircularProgressIndicator())
              : Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                Flexible(
                  flex: 1,
                  child: Text(widget.name, style: TextStyle(
                      fontSize: 18, fontWeight: FontWeight.bold)),
                ),
                Flexible(
                  flex: 4,
                  child: Center(
                    child: LayoutBuilder(builder: (context, constrains) =>
                        SizedBox(
                          width: constrains.maxHeight * 0.9,
                          height: constrains.maxHeight * 0.9,
                          child: Center(
                            child: Image.asset(imgPath, fit: BoxFit.fill)
                          ),
                        )
                    ),
                  ),
                ),
                Flexible(
                    flex: 1,
                    child: Center(
                      child: Text("$data $std", style: TextStyle(
                          fontSize: 18, fontWeight: FontWeight.w500)),
                    )
                )
              ]
                )
        );
      }
    );
  }
}

class DeviceStatus extends StatefulWidget {
  const DeviceStatus({super.key, required this.w, required this.h, required this.name, required this.path});

  final double w, h;
  final String name;
  final String path;

  @override
  State<DeviceStatus> createState() => _DeviceStatusState();
}
class _DeviceStatusState extends State<DeviceStatus> {

  late bool status;
  
  @override
  void initState() {
    super.initState();
  }
  
  @override
  Widget build(BuildContext context) {
    return StreamBuilder(
      stream: DBref.child(widget.path).onValue,
      builder: (context, snapshot) {
        if (snapshot.hasData) {
          status = snapshot.data!.snapshot.value as bool;
          print("${widget.path}: $status");
        }
        return IgnorePointer(
          ignoring: !snapshot.hasData,
          child: GestureDetector(
            onTap: deviceOnOff,
            child: Container(
              width: widget.w,
              height: widget.h,
              decoration: BoxDecoration(
                  color: snapshot.hasData && status
                      ? Colors.white : Colors.grey,
                  borderRadius: BorderRadius.circular(15),
                  boxShadow: snapshot.hasData && status
                      ? [BoxShadow(
                          color: Colors.grey.shade400,
                          blurRadius: 4,
                          spreadRadius: 2,
                          offset: Offset(2,2)
                        )]
                      : null
              ),
              child: snapshot.hasData
                ? Column(
                    crossAxisAlignment: CrossAxisAlignment.center,
                    children: [
                      Flexible(
                        flex: 1,
                        child: Align(
                          alignment: Alignment.bottomCenter,
                          child: Text(widget.name,
                            style: TextStyle(fontSize: 16, fontWeight: FontWeight.w500),
                        ))
                      ),
                      Flexible(
                        flex: 1,
                        child: Center(
                          child: Container(
                            padding: EdgeInsets.symmetric(
                                horizontal: 12, vertical: 3),
                            decoration: BoxDecoration(
                              color: status ? Colors.lightGreen : Colors.blueGrey,
                              borderRadius: BorderRadius.circular(5)
                            ),
                            child: Text(
                              status ? "On" : "Off",
                              style: TextStyle(color: Colors.white, fontSize: 14)
                            )
                          ),
                        )
                      )
                    ]
                  )
                : Center(child: CircularProgressIndicator())
            ),
          ),
        );
      },
    );
  }
  
  void deviceOnOff() {
    setState(() {
      status = !status;
      DBref.child(widget.path).set(status);
      sctr.add("${status ? "1" : "0"}${widget.name}");
    });
    print("${widget.path}: $status");
  }
}

class MessageBox extends StatefulWidget {
  const MessageBox({Key? key}) : super(key: key);

  @override
  State<MessageBox> createState() => _MessageBoxState();
}
class _MessageBoxState extends State<MessageBox> {
  String? name;
  bool onoff = false;
  double opacity = 0.0;
  Timer? timer;

  @override
  Widget build(BuildContext context) {
    return StreamBuilder<String>(
      stream: sctr.stream,
      builder: (context, snapshot) {
        if (snapshot.hasData) {
          if (snapshot.data!.isEmpty) {
            opacity = 0.0;
          }
          else {
            final data = snapshot.data!;
            name = data.substring(1);
            onoff = data[0] == '1';
            opacity = 1.0;
            if (timer != null) timer!.cancel();
            timer = Timer(Duration(milliseconds: 1500), () => sctr.add(''));
          }
        }
        return AnimatedOpacity(
          opacity: opacity,
          duration: Duration(milliseconds: 300),
          child: Container(
            padding: EdgeInsets.symmetric(horizontal: 24, vertical: 16),
            decoration: BoxDecoration(
              color: Colors.white,
              borderRadius: BorderRadius.circular(20),
              boxShadow: [
                BoxShadow(
                  color: Colors.grey,
                  blurRadius: 2,
                  spreadRadius: 2,
                  offset: Offset(2, 2),
                ),
              ],
            ),
            alignment: Alignment.center,
            child: Text(
              "$name 전원 ${onoff ? 'On' : 'Off'}",
              style: TextStyle(fontSize: 12, color: Colors.black),
            ),
          ),
        );
      },
    );
  }
}

