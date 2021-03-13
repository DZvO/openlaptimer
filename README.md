# openlaptimer
Cheap/affordable and simple lap timer.
Annoyed by your trackday-organizers not publishing laptimes or maybe they don't even provide a rentable laptimer? This is the project you've been looking for.

Includes datalogging to SD-Card so you can analyze what happened using a software that is currently WIP.

![this is how it looks](https://i.imgur.com/BmjMpmZ.png)

## Software
### Features
- Loads track start/finish-line definitions from SD card and automatically uses the closest by distance
- Shows best and current laptime while going faster than 5 km/h; shows best and cycles past laptimes while going slower/standing still
- Logs laptimes to SD card (laptimes.txt)
- Logs current GPS coordinates and IMU data to SD card (datalogging.txt)

### Libraries
TODO Add these libraries as git submodules  
- https://github.com/greiman/SdFat
- https://github.com/mikalhart/TinyGPS
- https://github.com/pololu/l3g-arduino
- https://github.com/pololu/LSM303-arduino

## Current (Prototype) Hardware
Mostly stuff I had on hand already:
- [HD44870 based 1602 LCD Display](https://www.aliexpress.com/item/32511014601.html?spm=a2g0o.productlist.0.0.5dc23129GNr4f8&algo_pvid=9d68e49d-5100-4d8e-94e7-a9c2f33ad5a9&algo_expid=9d68e49d-5100-4d8e-94e7-a9c2f33ad5a9-2&btsid=0b0a187b16126921761346588e857e&ws_ab_test=searchweb0_0,searchweb201602_,searchweb201603_))
- [Ublox Neo-6M/7M/8M or similar GPS-Module](https://www.aliexpress.com/item/1005001683579019.html?spm=a2g0o.productlist.0.0.45d228417CAAvz&algo_pvid=fd904115-b223-4756-958a-c9dcc493dcf8&algo_expid=fd904115-b223-4756-958a-c9dcc493dcf8-26&btsid=2100bdec16156406050744607e6274&ws_ab_test=searchweb0_0,searchweb201602_,searchweb201603_)
- [Teensy 3.0 or better](https://www.pjrc.com/store/teensy3.html)
- [micro SD module](https://www.aliexpress.com/item/32412677790.html?spm=a2g0o.productlist.0.0.264e2424KffOoQ&algo_pvid=0bb503af-d83b-4182-8ece-e1299e280228&algo_expid=0bb503af-d83b-4182-8ece-e1299e280228-2&btsid=0b0a050b16156411634622912ea2ad&ws_ab_test=searchweb0_0,searchweb201602_,searchweb201603_)
- [Pololu MinIMU v2](https://www.pololu.com/product/1268)

## Future (Release) Hardware (planned for 2022 Season)
Based on the results of the prototype a increment with the following hardware is planned:  
- [STM32 Board with integrated SD-Card-reader (8-15€)](https://www.aliexpress.com/item/1005001683272407.html?spm=a2g0o.productlist.0.0.153237aahiiSB4&algo_pvid=b1da5c32-ab62-47bc-a187-5b51d12699ca&algo_expid=b1da5c32-ab62-47bc-a187-5b51d12699ca-5&btsid=2100bddb16156403924028180edaa4&ws_ab_test=searchweb0_0,searchweb201602_,searchweb201603_) (only supports 3.3V!)
- [Ublox NEO-XM Module (5€)](https://www.aliexpress.com/item/1005001683579019.html?spm=a2g0o.productlist.0.0.45d228417CAAvz&algo_pvid=fd904115-b223-4756-958a-c9dcc493dcf8&algo_expid=fd904115-b223-4756-958a-c9dcc493dcf8-26&btsid=2100bdec16156406050744607e6274&ws_ab_test=searchweb0_0,searchweb201602_,searchweb201603_)
- [ILI9341 LCD Board (7€)](https://www.aliexpress.com/item/32960241206.html?spm=a2g0o.productlist.0.0.9b42504cM5HNyb&algo_pvid=83491f6b-666e-47f1-a3af-f5b0e2db98fe&algo_expid=83491f6b-666e-47f1-a3af-f5b0e2db98fe-7&btsid=2100bdf116156424844232120efaa1&ws_ab_test=searchweb0_0,searchweb201602_,searchweb201603_)
- [L3GD20+LSM303D IMU Module (3€)](https://www.aliexpress.com/item/32860106876.html?spm=a2g0o.productlist.0.0.2be1434ezctQPN&algo_pvid=387560b1-aa87-4f3b-bcc9-b182160d5fac&algo_expid=387560b1-aa87-4f3b-bcc9-b182160d5fac-2&btsid=2100bdf116156430579153192efa31&ws_ab_test=searchweb0_0,searchweb201602_,searchweb201603_)
- No custom PCB needed; only a few wires for interconnects: 1€  
- Case (PLA): 5€  

*Total: about 35€* 