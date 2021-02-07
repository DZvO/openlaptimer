#include <LiquidCrystalFast.h>
#include <TinyGPS.h>
#define MIN(x, y)       (((x) < (y)) ? (x) : (y))
#define MAX(x, y)       (((x) > (y)) ? (x) : (y))

LiquidCrystalFast lcd(2, 4, 3, 17, 16, 15, 14);
         // LCD pins: RS RW EN D4  D5  D6  D7
TinyGPS gps;
#define gpsPort Serial1

class Coordinate {
  public:
  float lat, lon;
  Coordinate() {}
  Coordinate(float la, float lo) { lat = la; lon = lo; }
};

class Line {
  public:
  Coordinate s, e;
  Line() {}
  Line(Coordinate a, Coordinate b) { s = a; e = b; }
};


static void smartdelay(unsigned long ms);
static void print_float(float val, float invalid, int len, int prec);
static void print_int(unsigned long val, unsigned long invalid, int len);
static void print_date(TinyGPS &gps);
static void print_str(const char *str, int len);

void printTimeToLCD(unsigned long t);
char get_line_intersection(float p0_x, float p0_y, float p1_x, float p1_y, 
    float p2_x, float p2_y, float p3_x, float p3_y, float *i_x, float *i_y);
bool doLinesIntersect(Line& a, Line& b);


Line startFinishLine = Line(Coordinate(48.06895, 11.56704), Coordinate(48.06842, 11.56613));

unsigned long laptimes[255];
unsigned char bestLapIndex = 0;
unsigned char currentLapIndex = 0;
unsigned long currentLapStartTimestamp = 0;
unsigned char cyclicalLapIndex = 1;

float prevlat, prevlon;
float lat, lon;
unsigned long age;

Coordinate debug_points[] = {
  Coordinate(48.07013, 11.56438),
  Coordinate(48.07290, 11.56828),
  Coordinate(48.07024, 11.57249),
  Coordinate(48.06750, 11.56859)
};

void setup() {
  lcd.begin(16, 2);

  lcd.print("pups racing team");
  lcd.setCursor(0, 1);
  lcd.print("~   laptimer   ~");

  Serial.begin(115200);
  gpsPort.begin(57600);

  //wait for fix
  smartdelay(5000);
  lcd.clear();
}

void printGPSDebugInfo() {
  static unsigned long lastPrint = millis();

  if(lastPrint + 1000 < millis()) {
    lastPrint = millis();
    unsigned long chars = 0;
    unsigned short sentences = 0, failed = 0;
    static const double LONDON_LAT = 51.508131, LONDON_LON = -0.128002;
    Serial.println("Sats HDOP Latitude  Longitude  Fix  Date       Time     Date Alt    Course Speed Card  Distance Course Card  Chars Sentences Checksum");
    Serial.println("          (deg)     (deg)      Age                      Age  (m)    --- from GPS ----  ---- to London  ----  RX    RX        Fail");
    Serial.println("-------------------------------------------------------------------------------------------------------------------------------------");
  
    print_int(gps.satellites(), TinyGPS::GPS_INVALID_SATELLITES, 5);
    print_int(gps.hdop(), TinyGPS::GPS_INVALID_HDOP, 5);
  
    print_float(lat, TinyGPS::GPS_INVALID_F_ANGLE, 10, 6);
    print_float(lon, TinyGPS::GPS_INVALID_F_ANGLE, 11, 6);
    print_int(age, TinyGPS::GPS_INVALID_AGE, 5);
    print_date(gps);
    print_float(gps.f_altitude(), TinyGPS::GPS_INVALID_F_ALTITUDE, 7, 2);
    print_float(gps.f_course(), TinyGPS::GPS_INVALID_F_ANGLE, 7, 2);
    print_float(gps.f_speed_kmph(), TinyGPS::GPS_INVALID_F_SPEED, 6, 2);
    print_str(gps.f_course() == TinyGPS::GPS_INVALID_F_ANGLE ? "*** " : TinyGPS::cardinal(gps.f_course()), 6);
    print_int(lat == TinyGPS::GPS_INVALID_F_ANGLE ? 0xFFFFFFFF : (unsigned long)TinyGPS::distance_between(lat, lon, LONDON_LAT, LONDON_LON) / 1000, 0xFFFFFFFF, 9);
    print_float(lat == TinyGPS::GPS_INVALID_F_ANGLE ? TinyGPS::GPS_INVALID_F_ANGLE : TinyGPS::course_to(lat, lon, LONDON_LAT, LONDON_LON), TinyGPS::GPS_INVALID_F_ANGLE, 7, 2);
    print_str(lat == TinyGPS::GPS_INVALID_F_ANGLE ? "*** " : TinyGPS::cardinal(TinyGPS::course_to(lat, lon, LONDON_LAT, LONDON_LON)), 6);
  
    gps.stats(&chars, &sentences, &failed);
    print_int(chars, 0xFFFFFFFF, 6);
    print_int(sentences, 0xFFFFFFFF, 10);
    print_int(failed, 0xFFFFFFFF, 9);
    Serial.println();
  }
}

void loop() {
  lcd.setCursor(0, 0);
  prevlat = lat;
  prevlon = lon;
  gps.f_get_position(&lat, &lon, &age);
  /*static int debugI = 0;
  static unsigned long debugT = millis();
  Serial.print("debugI: ");
  Serial.println(debugI);
  if(debugT + 5000 * random(5) < millis()) {
    debugT = millis();

    lat = debug_points[debugI].lat;
    lon = debug_points[debugI].lon;
    
    debugI++;
    if(debugI > 3) debugI = 0;
  }*/
  //printGPSDebugInfo();
  smartdelay(50);

  Line currentMove = Line(Coordinate(lat, lon), Coordinate(prevlat, prevlon));
  if(doLinesIntersect(currentMove, startFinishLine)) {
    laptimes[currentLapIndex] = millis() - currentLapStartTimestamp;
    currentLapStartTimestamp = millis();
    currentLapIndex++;
    
    //check for best lap TODO not necessary to go through whole list
    for(int i = 1; i < currentLapIndex; i++) {
      if(laptimes[i] < laptimes[bestLapIndex]) bestLapIndex = i;
    }
  }
  
  if(age > 500)  lcd.print("!");
  else           lcd.print(" ");

  if(currentLapIndex == 0) { //not driven through start/finish line yet - display accordingly
    lcd.print( "      B# -:--.-");
    lcd.setCursor(0, 1);
    lcd.print("       C# -:--.-");    
  } else { 
    float current_speed = 5; //gps.f_speed_kmph();
    static unsigned long debugT2 = millis();
    if(debugT2 + 10000 < millis()) {
      debugT2 = millis();
      current_speed *= -1;
    }
    
    if(current_speed < 2) {
      static unsigned long cycleTimer = millis();
      //display best lap and all lap times cyclical 
      lcd.print( "      B");
      lcd.print(bestLapIndex % 10);
      lcd.print(" ");
      printTimeToLCD(laptimes[bestLapIndex]);
      lcd.setCursor(0, 1);
      lcd.print("       L");
      lcd.print(cyclicalLapIndex % 10);
      lcd.print(" ");
      printTimeToLCD(laptimes[cyclicalLapIndex]); 
      if(cycleTimer + 1500 < millis()) {
        cyclicalLapIndex++;
        if(cyclicalLapIndex == currentLapIndex) cyclicalLapIndex = 1;
      }
    } else {
      //display best lap- and current lap-time
      lcd.print( "      B");
      lcd.print(bestLapIndex % 10);
      lcd.print(" ");
      printTimeToLCD(laptimes[bestLapIndex]);
      lcd.setCursor(0, 1);
      lcd.print("       C");
      lcd.print(currentLapIndex % 10);
      lcd.print(" ");
      printTimeToLCD(millis() - currentLapStartTimestamp); 
    }
  }  
  Serial.println("--------");
  smartdelay(50);
}

static void smartdelay(unsigned long ms)
{
  unsigned long start = millis();
  do {
    while (gpsPort.available())
      gps.encode(gpsPort.read());
  } while (millis() - start < ms);
}

static void print_float(float val, float invalid, int len, int prec)
{
  if (val == invalid) {
    while (len-- > 1)
      Serial.print('*');
    Serial.print(' ');
  } else {
    Serial.print(val, prec);
    int vi = abs((int)val);
    int flen = prec + (val < 0.0 ? 2 : 1); // . and -
    flen += vi >= 1000 ? 4 : vi >= 100 ? 3 : vi >= 10 ? 2 : 1;
    for (int i=flen; i<len; ++i)
      Serial.print(' ');
  }
  smartdelay(0);
}

static void print_int(unsigned long val, unsigned long invalid, int len)
{
  char sz[32];
  if (val == invalid) {
    strcpy(sz, "*******");
  } else {
    sprintf(sz, "%ld", val);
  }
  sz[len] = 0;
  for (int i=strlen(sz); i<len; ++i) {
    sz[i] = ' ';
  }
  if (len > 0) {
    sz[len-1] = ' ';
  }
  Serial.print(sz);
  smartdelay(0);
}

static void print_date(TinyGPS &gps)
{
  int year;
  byte month, day, hour, minute, second, hundredths;
  unsigned long age;
  gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths, &age);
  if (age == TinyGPS::GPS_INVALID_AGE) {
    Serial.print("********** ******** ");
  } else {
    char sz[32];
    sprintf(sz, "%02d/%02d/%02d %02d:%02d:%02d ",
        month, day, year, hour, minute, second);
    Serial.print(sz);
  }
  print_int(age, TinyGPS::GPS_INVALID_AGE, 5);
  smartdelay(0);
}

static void print_str(const char *str, int len)
{
  int slen = strlen(str);
  for (int i=0; i<len; ++i) {
    Serial.print(i<slen ? str[i] : ' ');
  }
  smartdelay(0);
}

void printTimeToLCD(unsigned long t) {
  long ms = t % 1000 / 100;
  long s = (t / 1000) % 60;
  long m = (t / (1000 * 60)) % 24;
  
  lcd.print(m);
  lcd.print(":");
  if(s < 10) lcd.print("0");
  lcd.print(s);
  lcd.print(".");
  lcd.print(ms);
}

bool LineIntersection(
  float x1, float y1, float x2, float y2,
  float sf1x, float sf1y, float sf2x, float sf2y
) {
  float z1, z2, z3, z4;
  float s1, s2, s3, s4;

    //quick rejection test
  if (!(MAX(sf1x, sf2x) >= MIN(x1, x2) && MAX(x1, x2) >= MIN(sf1x, sf2x) && 
        MAX(sf1y, sf2y) >= MIN(y1, y2) && MAX(y1, y2) >= MIN(sf1y, sf2y))) {
    return false;
  }
 
  //straddle tests
  if ((z1 = ((x1 - sf1x)*(sf2y - sf1y)) - ((y1 - sf1y)*(sf2x - sf1x))) < 0)
    s1 = -1; //counterclockwise
  else if (z1 > 0)
    s1 = 1;  //clockwise
  else
    s1 = 0;  //collinear
 
  if ((z2 = ((x2 - sf1x)*(sf2y - sf1y)) - ((y2 - sf1y)*(sf2x - sf1x))) < 0)
    s2 = -1;
  else if (z2 > 0)
    s2 = 1;
  else
    s2 = 0;
 
  if ((z3 = ((sf1x - x1)*(y2 - y1)) - ((sf1y - y1)*(x2 - x1))) < 0)
    s3 = -1;
  else if (z3 > 0)
    s3 = 1;
  else
    s3 = 0;
 
  if ((z4 = ((sf2x - x1)*(y2 - y1)) - ((sf2y - y1)*(x2 - x1))) < 0)
    s4 = -1;
  else if (z4 > 0)
    s4 = 1;
  else
    s4 = 0;
 
  if ((s1*s2 <= 0) && (s3*s4 <= 0)) {
    Serial.println("collision");
    return true;
  }
 
  //line segments do not intersect
  return false;
}

bool doLinesIntersect(Line& a, Line& b) {
    return LineIntersection(
      a.s.lat, a.s.lon, a.e.lat, a.e.lon,
      b.s.lat, b.s.lon, b.e.lat, b.e.lon
    );
}
