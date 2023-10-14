class AnalogJoystick {
public:
    AnalogJoystick(int xpin, int ypin, int bpin, int calValue);
    AnalogJoystick(int xpin, int ypin, int bpin);
    int getValue() {
        return 0;
    }
    bool isButtonPressed() {
        // Szimuláljuk a gomb lenyomását az analóg joystick középső pozíciójának vizsgálatával
        int sensorValue = digitalRead(BPin); 

        return (sensorValue); // A 100 egy tetszőleges küszöbérték, amitől már gombnyomásnak tekintjük
    }
    inline int getDirection();
private:
    int XPin,YPin,BPin;
    int jCalibration = 2250;
    int thereshold = 500;
};


