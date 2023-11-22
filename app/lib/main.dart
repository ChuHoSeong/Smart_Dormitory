import 'dart:async';

import 'package:firebase_core/firebase_core.dart';
import 'package:firebase_database/firebase_database.dart';
import 'package:flutter/material.dart';
import 'firebase_options.dart';

Future<void> main() async {
  WidgetsFlutterBinding.ensureInitialized();
  await Firebase.initializeApp(options: DefaultFirebaseOptions.currentPlatform);
  runApp(MyApp());
}

class MyApp extends StatelessWidget {
  // This widget is the root of your application.
  @override
  Widget build(BuildContext context) {
    return MaterialApp(
        debugShowCheckedModeBanner: false,
        title: 'Flutter Demo',
        theme: ThemeData(
          primarySwatch: Colors.purple,
          visualDensity: VisualDensity.adaptivePlatformDensity,
        ),
        home: HomePage());
  }
}

class HomePage extends StatefulWidget {
  @override
  _HomePageState createState() => _HomePageState();
}

class _HomePageState extends State<HomePage> {

  final DBref = FirebaseDatabase.instance.ref();
  bool ledStatus = true;
  bool isLoading = false;

  void getLEDStatus() async {
    await DBref.child('LED_STATUS').once().then((value) {
      ledStatus = value.snapshot.value as bool;
      print(ledStatus);
    },
    onError: (e) {
      print(e);
    });

    setState(() {
      isLoading = false;
    });
  }

  @override
  void initState() {
    isLoading = true;
    getLEDStatus();
    super.initState();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text(
          'IOT App',
        ),
      ),
      body: Center(
        child: isLoading
            ? CircularProgressIndicator()
            : ElevatedButton(
          child: Text(
            ledStatus == true ? 'On' : 'Off',
            style: TextStyle(
                fontSize: 20, fontWeight: FontWeight.w300),
          ),
          onPressed: () {
            buttonPressed();
          },
        ),
      ),
    );
  }

  void buttonPressed() {
    if (ledStatus == false) {
      DBref.child('LED_STATUS').set(true);
      setState(() {
        ledStatus = true;
        print(ledStatus);
      });
    } else {
      DBref.child('LED_STATUS').set(false);
      setState(() {
        ledStatus = false;
        print(ledStatus);
      });
    }
  }
}