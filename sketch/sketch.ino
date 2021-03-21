#include <LiquidCrystalFast.h>
#include <TinyGPS.h>
#include <Wire.h>
#include <LSM303.h>
#include <L3G.h>
#include <SPI.h>
#include <SdFat.h>

#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define SPI_SPEED SD_SCK_MHZ(4)
#define SD_CHIPSELECT 10
//#define DEBUG
//#define SIMULATE

LiquidCrystalFast lcd(2, 4, 3, 17, 16, 15, 14);
         // LCD pins: RS RW EN D4  D5  D6  D7
TinyGPS gps;
#define gpsPort Serial1

L3G gyro;
LSM303 compass;

SdFs sd;
FsFile laptimesFile;
FsFile dataloggingFile;

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

#ifdef DEBUG
void printGPSDebugInfo();
static void print_float(float val, float invalid, int len, int prec);
static void print_int(unsigned long val, unsigned long invalid, int len);
static void print_date(TinyGPS &gps);
static void print_str(const char *str, int len);
#endif

void timeToString(char* buffer, unsigned long t);
char timeBuffer[16];
char get_line_intersection(float p0_x, float p0_y, float p1_x, float p1_y, 
    float p2_x, float p2_y, float p3_x, float p3_y, float *i_x, float *i_y);
bool doLinesIntersect(Line& a, Line& b);

Line startFinishLine = Line(Coordinate(48.06895, 11.56704), Coordinate(48.06842, 11.56613));

unsigned long laptimes[128];
unsigned char bestLapIndex = 1;
unsigned char currentLapIndex = 0;
unsigned long currentLapStartTimestamp = 0;
unsigned char cyclicalLapIndex = 1;
unsigned long lastDataLogTimestamp = 0;
String dataloggingLine = "";

float prevlat, prevlon;
float lat, lon;
unsigned long age;

void setup() {
  for(int i = 0; i < 128; i++) laptimes[i] = 0;

  #if defined(DEBUG) || defined(SIMULATE)
  Serial.begin(115200);
  #endif
  
  lcd.begin(16, 2);

  lcd.clear();
  lcd.print("dzvo racing team");
  lcd.setCursor(0, 1);
  lcd.print("~   laptimer   ~");

  gpsPort.begin(57600);
  smartdelay(2000);

  #ifndef SIMULATE
    unsigned char i = 0;
    do {
      gps.f_get_position(&lat, &lon, &age);
      smartdelay(500);
      lcd.setCursor(0, 1);
      if(i % 4 == 0)      lcd.print("wait for gps    ");
      else if(i % 4== 1)  lcd.print("wait for gps .  ");
      else if(i % 4 == 2) lcd.print("wait for gps .. ");
      else if(i % 4 == 3) lcd.print("wait for gps ...");
      i++;
    } while(age >= TinyGPS::GPS_INVALID_AGE); //wait until we have a gps fix

    lcd.setCursor(0, 1);
    lcd.print("opening sd card ");
    smartdelay(100);

    if (!sd.begin(SD_CHIPSELECT, SPI_SPEED)) {
      lcd.clear();
      lcd.print("error with");
      lcd.setCursor(0, 1);
      lcd.print("sd card");
      while(true) {}
    }
  #else 
    lcd.setCursor(0, 1);
    lcd.print("opening sd card ");
    smartdelay(100);

    if (!sd.begin(SD_CHIPSELECT, SPI_SPEED)) {
      lcd.clear();
      lcd.print("error with");
      lcd.setCursor(0, 1);
      lcd.print("sd card");
      while(true) {}
    }
    FsFile simulationFile = sd.open("simulation.txt", FILE_READ);
    if (simulationFile) {
      String line = simulationFile.readStringUntil('\n');
      String slat = line.substring(0, line.indexOf(','));
      line = line.substring(line.indexOf(','));
      String slon = line.substring(1);
      lat = slat.toFloat();
      lon = slon.toFloat();
      simulationFile.close();
    } else {
      Serial.println("simul.txt error");
    }
  #endif


  lcd.setCursor(0, 1);
  lcd.print("open tracks.txt ");

  FsFile trackDefinitionsFile = sd.open("tracks.txt", FILE_READ);
  if (trackDefinitionsFile) {
    float minDist = -1;
    String minTrack = "none";
    // TODO this should probably reside in a separate method
    while(trackDefinitionsFile.available()) {
      String line = trackDefinitionsFile.readStringUntil('\n');
      String trackname = line.substring(0, line.indexOf(','));
      line = line.substring(line.indexOf(',') + 1);
      float lat1 = line.substring(0, line.indexOf(',')).toFloat();
      line = line.substring(line.indexOf(',') + 1);
      float lon1 = line.substring(0, line.indexOf(',')).toFloat();
      line = line.substring(line.indexOf(',') + 1);
      float lat2 = line.substring(0, line.indexOf(',')).toFloat();
      line = line.substring(line.indexOf(',') + 1);
      float lon2 = line.substring(0, line.indexOf(',')).toFloat();

      smartdelay(10);

      float dist = TinyGPS::distance_between(lat, lon, lat1, lon1);
      if(minDist == -1 || dist < minDist) {
        minDist = dist;
        minTrack = trackname;
        startFinishLine = Line(Coordinate(lat1, lon1), Coordinate(lat2, lon2));
      }
    }
    trackDefinitionsFile.close();

    lcd.setCursor(0, 0);
    lcd.print("using track:    ");
    lcd.setCursor(0, 1);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    lcd.print(minTrack.substring(0, 16));
    smartdelay(1000);
  } else {
    lcd.setCursor(0, 0);
    lcd.print("error opening:  ");
    lcd.setCursor(0, 1);
    lcd.print("tracks.txt      ");
    while(true) {}
  }

  dataloggingFile = sd.open("datalogging.txt", FILE_WRITE);
  if(!dataloggingFile) {
    lcd.setCursor(0, 0);
    lcd.print("error opening:  ");
    lcd.setCursor(0, 1);
    lcd.print("datalogging.txt ");
    while(true) {}
  }
  dataloggingFile.close();

  laptimesFile = sd.open("laptimes.txt", FILE_WRITE);
  if(!laptimesFile) {
    lcd.setCursor(0, 0);
    lcd.print("error opening:  ");
    lcd.setCursor(0, 1);
    lcd.print("laptimes.txt    ");
    while(true) {}
  }
  laptimesFile.close();

  Wire.begin();
  if (!gyro.init())
  {
    lcd.setCursor(0, 0);
    lcd.print("error opening:  ");
    lcd.setCursor(0, 1);
    lcd.print("gyro            ");
    while (true);
  }
  gyro.enableDefault();

  if (!compass.init())
  {
    lcd.setCursor(0, 0);
    lcd.print("error opening:  ");
    lcd.setCursor(0, 1);
    lcd.print("acc/mag         ");
    while (true);
  }
  compass.enableDefault();
}

void loop() {
  lcd.setCursor(0, 0);
  prevlat = lat;
  prevlon = lon;
  #ifndef SIMULATE
    gps.f_get_position(&lat, &lon, &age);
  #else
    static unsigned int skipper = 0;
    FsFile simulationFile = sd.open("simulation.txt", FILE_READ);
    if(skipper >= simulationFile.size() - 1000) skipper = 0;
    simulationFile.seek(skipper);
    String line = simulationFile.readStringUntil('\n');
    skipper = simulationFile.position();
    String slat = line.substring(0, line.indexOf(','));
    String slon = line.substring(line.indexOf(',') + 1);
    lat = slat.toFloat();
    lon = slon.toFloat();
    Serial.print(lat, 6);
    Serial.print(",");
    Serial.println(lon, 6);
    Serial.println();
    simulationFile.close();
    smartdelay(random(1, 5));
  #endif

  #ifdef DEBUG
    printGPSDebugInfo();
  #endif

  #ifndef SIMULATE
    smartdelay(50);
  #endif

  Line currentMove = Line(Coordinate(prevlat, prevlon), Coordinate(lat, lon));
  if(doLinesIntersect(currentMove, startFinishLine)) {
    laptimes[currentLapIndex] = millis() - currentLapStartTimestamp;
    currentLapStartTimestamp = millis();

    if(currentLapIndex != 0) {
      char buffer[32];
      timeToString(timeBuffer, laptimes[currentLapIndex]);
      sprintf(buffer, "Lap# %d %s", currentLapIndex, timeBuffer);

      laptimesFile = sd.open("laptimes.txt", FILE_WRITE);
      laptimesFile.println(buffer);
      laptimesFile.close();

      if(laptimes[currentLapIndex] < laptimes[bestLapIndex]) bestLapIndex = currentLapIndex;
    }
    currentLapIndex++;
  }
  
  if(age > 500)  lcd.print("!");
  else           lcd.print(" ");

  char speedBuffer [4];
  sprintf(speedBuffer, "%3d", (int)gps.f_speed_kmph());

  if(currentLapIndex == 0) { //not driven through start/finish line yet
    static unsigned char cycle = 0;
    lcd.print(speedBuffer);
    lcd.print("            ");  
    lcd.setCursor(0, 1);
    if(cycle++ < 16) { 
      lcd.print("                ");  
    } else {
      lcd.print("               ~");
    }
    if(cycle > 32) cycle = 0;
  } else { 
    lcd.print(speedBuffer);
    lcd.print(bestLapIndex < 10 ? "   B" : "  B");
    lcd.print(bestLapIndex); lcd.print(" ");
    timeToString(timeBuffer, laptimes[bestLapIndex]);
    lcd.print(timeBuffer);
    lcd.setCursor(0, 1);

    if(gps.f_speed_kmph() < 4) {
      static unsigned long cycleTimer = millis();
      if(cycleTimer + 2500 < millis()) {
        cycleTimer = millis();
        cyclicalLapIndex++;
        if(cyclicalLapIndex >= currentLapIndex) cyclicalLapIndex = 1;
      }
      //display best lap and all lap times cyclical 
      lcd.print(cyclicalLapIndex < 10 ? "       L" : "      L");
      lcd.print(cyclicalLapIndex); lcd.print(" ");
      timeToString(timeBuffer, laptimes[cyclicalLapIndex]);
      lcd.print(timeBuffer); 
    } else {
      //display best lap- and current lap-time
      lcd.print(currentLapIndex < 10 ? "       C" : "      C");
      lcd.print(currentLapIndex); lcd.print(" ");
      timeToString(timeBuffer, millis() - currentLapStartTimestamp);
      lcd.print(timeBuffer); 
    }
  }
#ifndef SIMULATE
  if(lastDataLogTimestamp + 250 < millis()) {
    lastDataLogTimestamp = millis();
    int year;
    unsigned char month, day, hour, minute, second, hundredths;
    unsigned long age;
    gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths, &age);
    
    gyro.read();
    compass.read();

    dataloggingLine += millis();          dataloggingLine += ";";
    dataloggingLine += age;               dataloggingLine += ";";
    dataloggingLine += String(year) + "-" + String(month, 2) + "-" + String(day, 2) + " " +
            String(hour, 2) + ":" + String(minute, 2) + ":" + String(second, 2) + "." + String(hundredths) + ",";
    dataloggingLine += String(lat, 6);    dataloggingLine += ",";
    dataloggingLine += String(lon, 6);    dataloggingLine += ";";
    dataloggingLine += String((int)gps.f_speed_kmph(), 3); dataloggingLine += ";";
    dataloggingLine += (int)gyro.g.x;     dataloggingLine += ",";
    dataloggingLine += (int)gyro.g.y;     dataloggingLine += ",";
    dataloggingLine += (int)gyro.g.z;     dataloggingLine += ",";
    dataloggingLine += (int)compass.a.x;  dataloggingLine += ",";
    dataloggingLine += (int)compass.a.y;  dataloggingLine += ",";
    dataloggingLine += (int)compass.a.z;  dataloggingLine += ",";
    dataloggingLine += (int)compass.m.x;  dataloggingLine += ",";
    dataloggingLine += (int)compass.m.y;  dataloggingLine += ",";
    dataloggingLine += (int)compass.m.z;  dataloggingLine += ";";
    dataloggingLine += "\n";

    static unsigned char ctr = 0;
    if(ctr++ > 4) {
      ctr = 0;
      dataloggingFile = sd.open("datalogging.txt", FILE_WRITE);;
      dataloggingFile.print(dataloggingLine);
      dataloggingFile.close();
      dataloggingLine = "";
      lcd.setCursor(0, 1);
      lcd.print(".");
    } else {
      lcd.setCursor(0, 1);
      lcd.print(" ");
    }
  }
  smartdelay(50);
#endif
}

static void smartdelay(unsigned long ms)
{
  unsigned long start = millis();
  do {
    while (gpsPort.available())
      gps.encode(gpsPort.read());
  } while (millis() - start < ms);
}

#ifdef DEBUG
void printGPSDebugInfo() {
  static unsigned long lastPrint = millis();

  if(lastPrint + 1000 < millis()) {
    lastPrint = millis();
    unsigned long chars = 0;
    unsigned short sentences = 0, failed = 0;
    Serial.println("Sats HDOP Latitude  Longitude  Fix  Date       Time     Date Alt    Course Speed Card Chars Sentences Checksum");
    Serial.println("          (deg)     (deg)      Age                      Age  (m)    --- from GPS ---   RX    RX        Fail   ");
    Serial.println("--------------------------------------------------------------------------------------------------------------");
  
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
  
    gps.stats(&chars, &sentences, &failed);
    print_int(chars, 0xFFFFFFFF, 6);
    print_int(sentences, 0xFFFFFFFF, 10);
    print_int(failed, 0xFFFFFFFF, 9);
    Serial.println();
  }
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
#endif

void timeToString(char* buffer, unsigned long t) {
  short ms = t % 1000 / 100;
  short s = (t / 1000) % 60;
  short m = (t / (1000 * 60)) % 24;
  sprintf(buffer, "%d:%02d.%d", m, s, ms);
  /*String ret = "";
  ret += m;
  ret += ":";
  if(s < 10) ret += "0";
  ret += s;
  ret += ".";
  ret += ms;
  return ret;*/
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
