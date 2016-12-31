#include <pebble.h>
#include <math.h>
#include "settings.h"
#include "weather.h"
#include "languages.h"
#include "util.h"
#ifdef PBL_HEALTH
#include "health.h"
#endif
#include "sidebar_widgets.h"

bool SidebarWidgets_useCompactMode = false;
bool SidebarWidgets_fixedHeight = false;
int SidebarWidgets_xOffset;

// sidebar icons
GDrawCommandImage* dateImage;
GDrawCommandImage* disconnectImage;
GDrawCommandImage* batteryImage;
GDrawCommandImage* batteryChargeImage;

// fonts
GFont smSidebarFont;
GFont mdSidebarFont;
GFont lgSidebarFont;
GFont currentSidebarFont;
GFont currentSidebarSmallFont;
GFont batteryFont;

// the date, time and weather strings
char currentDayName[8];
char currentDayNum[5];
char currentMonth[8];
char currentWeekNum[5];
char currentSecondsNum[5];
char altClock[8];
char currentBeats[5];

// the widgets
SidebarWidget batteryMeterWidget;
int BatteryMeter_getHeight();
void BatteryMeter_draw(GContext* ctx, int xPosition, int yPosition);

SidebarWidget emptyWidget;
int EmptyWidget_getHeight();
void EmptyWidget_draw(GContext* ctx, int xPosition, int yPosition);

SidebarWidget dateWidget;
int DateWidget_getHeight();
void DateWidget_draw(GContext* ctx, int xPosition, int yPosition);

SidebarWidget currentWeatherWidget;
int CurrentWeather_getHeight();
void CurrentWeather_draw(GContext* ctx, int xPosition, int yPosition);

SidebarWidget weatherForecastWidget;
int WeatherForecast_getHeight();
void WeatherForecast_draw(GContext* ctx, int xPosition, int yPosition);

SidebarWidget btDisconnectWidget;
int BTDisconnect_getHeight();
void BTDisconnect_draw(GContext* ctx, int xPosition, int yPosition);

SidebarWidget weekNumberWidget;
int WeekNumber_getHeight();
void WeekNumber_draw(GContext* ctx, int xPosition, int yPosition);

SidebarWidget secondsWidget;
int Seconds_getHeight();
void Seconds_draw(GContext* ctx, int xPosition, int yPosition);

SidebarWidget altTimeWidget;
int AltTime_getHeight();
void AltTime_draw(GContext* ctx, int xPosition, int yPosition);

SidebarWidget beatsWidget;
int Beats_getHeight();
void Beats_draw(GContext* ctx, int xPosition, int yPosition);

#ifdef PBL_HEALTH
  GDrawCommandImage* sleepImage;
  GDrawCommandImage* stepsImage;
  GDrawCommandImage* heartImage;

  SidebarWidget healthWidget;
  int Health_getHeight();
  void Health_draw(GContext* ctx, int xPosition, int yPosition);
  void Sleep_draw(GContext* ctx, int xPosition, int yPosition);
  void Steps_draw(GContext* ctx, int xPosition, int yPosition);

  SidebarWidget heartRateWidget;
  int HeartRate_getHeight();
  void HeartRate_draw(GContext* ctx, int xPosition, int yPosition);
#endif

void SidebarWidgets_init() {
  // load fonts
  smSidebarFont = fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD);
  mdSidebarFont = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
  lgSidebarFont = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);

  // load the sidebar graphics
  dateImage = gdraw_command_image_create_with_resource(RESOURCE_ID_DATE_BG);
  disconnectImage = gdraw_command_image_create_with_resource(RESOURCE_ID_DISCONNECTED);
  batteryImage = gdraw_command_image_create_with_resource(RESOURCE_ID_BATTERY_BG);
  batteryChargeImage = gdraw_command_image_create_with_resource(RESOURCE_ID_BATTERY_CHARGE);

  #ifdef PBL_HEALTH
    sleepImage = gdraw_command_image_create_with_resource(RESOURCE_ID_HEALTH_SLEEP);
    stepsImage = gdraw_command_image_create_with_resource(RESOURCE_ID_HEALTH_STEPS);
    heartImage = gdraw_command_image_create_with_resource(RESOURCE_ID_HEALTH_HEART);
  #endif

  // set up widgets' function pointers correctly
  batteryMeterWidget.getHeight = BatteryMeter_getHeight;
  batteryMeterWidget.draw      = BatteryMeter_draw;

  emptyWidget.getHeight = EmptyWidget_getHeight;
  emptyWidget.draw      = EmptyWidget_draw;

  dateWidget.getHeight = DateWidget_getHeight;
  dateWidget.draw      = DateWidget_draw;

  currentWeatherWidget.getHeight = CurrentWeather_getHeight;
  currentWeatherWidget.draw      = CurrentWeather_draw;

  weatherForecastWidget.getHeight = WeatherForecast_getHeight;
  weatherForecastWidget.draw      = WeatherForecast_draw;

  btDisconnectWidget.getHeight = BTDisconnect_getHeight;
  btDisconnectWidget.draw      = BTDisconnect_draw;

  weekNumberWidget.getHeight = WeekNumber_getHeight;
  weekNumberWidget.draw      = WeekNumber_draw;

  secondsWidget.getHeight = Seconds_getHeight;
  secondsWidget.draw      = Seconds_draw;

  altTimeWidget.getHeight = AltTime_getHeight;
  altTimeWidget.draw      = AltTime_draw;

  #ifdef PBL_HEALTH
    healthWidget.getHeight = Health_getHeight;
    healthWidget.draw = Health_draw;
    
    heartRateWidget.getHeight = HeartRate_getHeight;
    heartRateWidget.draw = HeartRate_draw;
  #endif

  beatsWidget.getHeight = Beats_getHeight;
  beatsWidget.draw      = Beats_draw;

}

void SidebarWidgets_deinit() {
  gdraw_command_image_destroy(dateImage);
  gdraw_command_image_destroy(disconnectImage);
  gdraw_command_image_destroy(batteryImage);
  gdraw_command_image_destroy(batteryChargeImage);

  #ifdef PBL_HEALTH
    gdraw_command_image_destroy(stepsImage);
    gdraw_command_image_destroy(sleepImage);
  #endif
}

void SidebarWidgets_updateFonts() {
  if(globalSettings.useLargeFonts) {
    currentSidebarFont = lgSidebarFont;
    currentSidebarSmallFont = mdSidebarFont;
    batteryFont = lgSidebarFont;
  } else {
    currentSidebarFont = mdSidebarFont;
    currentSidebarSmallFont = smSidebarFont;
    batteryFont = smSidebarFont;
  }
}

// c can't do true modulus on negative numbers, apparently
// from http://stackoverflow.com/questions/11720656/modulo-operation-with-negative-numbers
int mod(int a, int b) {
    int r = a % b;
    return r < 0 ? r + b : r;
}

void SidebarWidgets_updateTime(struct tm* timeInfo) {
  // APP_LOG(APP_LOG_LEVEL_DEBUG,"Current RAM: %d", heap_bytes_free());

  // set all the date strings
  strftime(currentDayNum,  3, "%e", timeInfo);
  strftime(currentWeekNum, 3, "%V", timeInfo);

  // remove padding on date num, if needed
  if(currentDayNum[0] == ' ') {
    currentDayNum[0] = currentDayNum[1];
    currentDayNum[1] = '\0';
  }

  // set the seconds string
  strftime(currentSecondsNum, 4, ":%S", timeInfo);

  strncpy(currentDayName, dayNames[globalSettings.languageId][timeInfo->tm_wday], sizeof(currentDayName));
  strncpy(currentMonth, monthNames[globalSettings.languageId][timeInfo->tm_mon], sizeof(currentMonth));

  if(globalSettings.enableAltTimeZone) {
    // set the alternate time zone string
    int hour = timeInfo->tm_hour;

    // apply the configured offset value
    hour += globalSettings.altclockOffset;

    char am_pm;

    // format it
    if(clock_is_24h_style()) {
      hour = mod(hour, 24);
      am_pm = (char) 0;
    } else {
      hour = mod(hour, 12);
      if(hour == 0) {
        hour = 12;
      }
      am_pm = (mod(hour, 24) < 12) ? 'a' : 'p';
    }

    if(globalSettings.showLeadingZero && hour < 10) {
      snprintf(altClock, sizeof(altClock), "0%i%c", hour, am_pm);
    } else {
      snprintf(altClock, sizeof(altClock), "%i%c", hour, am_pm);
    }
  }

  if(globalSettings.enableBeats) {
    // this must be last, because time_get_beats screws with the time structure
    int beats = 0;

    // set the swatch internet time beats
    beats = time_get_beats(timeInfo);
  
    snprintf(currentBeats, sizeof(currentBeats), "%i", beats);
  }
}

/* Sidebar Widget Selection */
SidebarWidget getSidebarWidgetByType(SidebarWidgetType type) {
  switch(type) {
    case BATTERY_METER:
      return batteryMeterWidget;
      break;
    case BLUETOOTH_DISCONNECT:
      return btDisconnectWidget;
      break;
    case DATE:
      return dateWidget;
      break;
    case ALT_TIME_ZONE:
      return altTimeWidget;
      break;
    case SECONDS:
      return secondsWidget;
      break;
    case WEATHER_CURRENT:
      return currentWeatherWidget;
      break;
    case WEATHER_FORECAST_TODAY:
      return weatherForecastWidget;
      break;
    case WEEK_NUMBER:
      return weekNumberWidget;
    #ifdef PBL_HEALTH
      case HEALTH:
        return healthWidget;
      case HEARTRATE:
        return heartRateWidget;
    #endif
    case BEATS:
      return beatsWidget;
    default:
      return emptyWidget;
      break;
  }
}

/********** functions for the empty widget **********/
int EmptyWidget_getHeight() {
  return 0;
}

void EmptyWidget_draw(GContext* ctx, int xPosition, int yPosition) {
  return;
}

/********** functions for the battery meter widget **********/

int BatteryMeter_getHeight() {
  BatteryChargeState chargeState = battery_state_service_peek();

  if(SidebarWidgets_fixedHeight) {
    return FIXED_WIDGET_HEIGHT;
  } else if(chargeState.is_charging || !globalSettings.showBatteryPct) {
    return 14; // graphic only height
  } else {
    return (globalSettings.useLargeFonts) ? 33 : 27; // heights with text
  }
}

void BatteryMeter_draw(GContext* ctx, int xPosition, int yPosition) {

  BatteryChargeState chargeState = battery_state_service_peek();
  uint8_t battery_percent = (chargeState.charge_percent > 0) ? chargeState.charge_percent : 5;

  graphics_context_set_text_color(ctx, globalSettings.sidebarTextColor);

  char batteryString[6];
  int batteryPositionY = yPosition;

  if(SidebarWidgets_fixedHeight){
    if(!globalSettings.showBatteryPct || chargeState.is_charging) {
      batteryPositionY += (FIXED_WIDGET_HEIGHT / 2) - 12;
    } else {
      batteryPositionY += 3;
    }
  } else {
    // correct for vertical empty space on battery icon
    batteryPositionY -= 5;
  }

  if (batteryImage) {
    gdraw_command_image_recolor(batteryImage, globalSettings.iconFillColor, globalSettings.iconStrokeColor);
    gdraw_command_image_draw(ctx, batteryImage, GPoint(xPosition + 3 + SidebarWidgets_xOffset, batteryPositionY));
  }

  if(chargeState.is_charging) {
    if(batteryChargeImage) {
      // the charge "bolt" icon uses inverted colors
      gdraw_command_image_recolor(batteryChargeImage, globalSettings.iconStrokeColor, globalSettings.iconFillColor);
      gdraw_command_image_draw(ctx, batteryChargeImage, GPoint(xPosition + 3 + SidebarWidgets_xOffset, batteryPositionY));
    }
  } else {

    int width = roundf(18 * battery_percent / 100.0f);

    graphics_context_set_fill_color(ctx, globalSettings.iconStrokeColor);

    #ifdef PBL_COLOR
      if(battery_percent <= 20) {
        graphics_context_set_fill_color(ctx, GColorRed);
      }
    #endif

    graphics_fill_rect(ctx, GRect(xPosition + 6 + SidebarWidgets_xOffset, 8 + batteryPositionY, width, 8), 0, GCornerNone);
  }

  // never show battery % while charging, because of this issue:
  // https://github.com/freakified/TimeStylePebble/issues/11
  if(globalSettings.showBatteryPct && !chargeState.is_charging) {
    int textOffsetY;

    if(!globalSettings.useLargeFonts) {
      if(SidebarWidgets_fixedHeight) {
        textOffsetY = 25;
      } else {
        textOffsetY = 18;
      }

      // put the percent sign on the opposite side if turkish
      snprintf(batteryString, sizeof(batteryString),
               (globalSettings.languageId == LANGUAGE_TR) ? "%%%d" : "%d%%",
               battery_percent);
    } else {
      if(SidebarWidgets_fixedHeight) {
        textOffsetY = 18;
      } else {
        textOffsetY = 14;
      }

      snprintf(batteryString, sizeof(batteryString), "%d", battery_percent);
    }
    graphics_draw_text(ctx,
                       batteryString,
                       batteryFont,
                       GRect(xPosition - 4 + SidebarWidgets_xOffset, textOffsetY + batteryPositionY, 38, 20),
                       GTextOverflowModeFill,
                       GTextAlignmentCenter,
                       NULL);
  }
}

/********** current date widget **********/

int DateWidget_getHeight() {
  if(globalSettings.useLargeFonts) {
    return (SidebarWidgets_useCompactMode) ? 42 : 62;
  } else  {
    return (SidebarWidgets_useCompactMode) ? 41 : 58;
  }
}

void DateWidget_draw(GContext* ctx, int xPosition, int yPosition) {
  graphics_context_set_text_color(ctx, globalSettings.sidebarTextColor);

  // compensate for extra space that appears on the top of the date widget
  yPosition -= (globalSettings.useLargeFonts) ? 10 : 7;

  // first draw the day name
  graphics_draw_text(ctx,
                     currentDayName,
                     currentSidebarFont,
                     GRect(xPosition - 5 + SidebarWidgets_xOffset, yPosition, 40, 20),
                     GTextOverflowModeFill,
                     GTextAlignmentCenter,
                     NULL);

  // next, draw the date background
  // (an image in normal mode, a rectangle in large font mode)
  if(!globalSettings.useLargeFonts) {
    if(dateImage) {
      gdraw_command_image_recolor(dateImage, globalSettings.iconFillColor, globalSettings.iconStrokeColor);
      gdraw_command_image_draw(ctx, dateImage, GPoint(xPosition + 3 + SidebarWidgets_xOffset, yPosition + 23));
    }
  } else {
    graphics_context_set_fill_color(ctx, globalSettings.iconStrokeColor);
    graphics_fill_rect(ctx, GRect(xPosition + 2 + SidebarWidgets_xOffset, yPosition + 30, 26, 22), 2, GCornersAll);

    graphics_context_set_fill_color(ctx, globalSettings.iconFillColor);
    graphics_fill_rect(ctx, GRect(xPosition + 4 + SidebarWidgets_xOffset, yPosition + 32, 22, 18), 0, GCornersAll);
  }

  // next, draw the date number
  graphics_context_set_text_color(ctx, globalSettings.iconStrokeColor);

  int yOffset = 0;
  yOffset = globalSettings.useLargeFonts ? 24 : 26;

  graphics_draw_text(ctx,
                     currentDayNum,
                     currentSidebarFont,
                     GRect(xPosition - 5 + SidebarWidgets_xOffset, yPosition + yOffset, 40, 20),
                     GTextOverflowModeFill,
                     GTextAlignmentCenter,
                     NULL);


   // switch back to normal color for the rest
  graphics_context_set_text_color(ctx, globalSettings.sidebarTextColor);

  // don't draw the month if we're in compact mode
  if(!SidebarWidgets_useCompactMode) {
    yOffset = globalSettings.useLargeFonts ? 48 : 47;

    graphics_draw_text(ctx,
                       currentMonth,
                       currentSidebarFont,
                       GRect(xPosition - 5 + SidebarWidgets_xOffset, yPosition + yOffset, 40, 20),
                       GTextOverflowModeFill,
                       GTextAlignmentCenter,
                       NULL);
  }


}

/********** current weather widget **********/

int CurrentWeather_getHeight() {
  if(globalSettings.useLargeFonts) {
    return 44;
  } else {
    return 42;
  }
}

void CurrentWeather_draw(GContext* ctx, int xPosition, int yPosition) {
  graphics_context_set_text_color(ctx, globalSettings.sidebarTextColor);

  if (Weather_currentWeatherIcon) {
    gdraw_command_image_recolor(Weather_currentWeatherIcon, globalSettings.iconFillColor, globalSettings.iconStrokeColor);
    gdraw_command_image_draw(ctx, Weather_currentWeatherIcon, GPoint(xPosition + 3 + SidebarWidgets_xOffset, yPosition));
  }

  // draw weather data only if it has been set
  if(Weather_weatherInfo.currentTemp != INT32_MIN) {

    int currentTemp = Weather_weatherInfo.currentTemp;

    if(!globalSettings.useMetric) {
      currentTemp = roundf(currentTemp * 1.8f + 32);
    }

    char tempString[8];

    // in large font mode, omit the degree symbol and move the text
    if(!globalSettings.useLargeFonts) {
      snprintf(tempString, sizeof(tempString), " %d°", currentTemp);

      graphics_draw_text(ctx,
                         tempString,
                         currentSidebarFont,
                         GRect(xPosition - 5 + SidebarWidgets_xOffset, yPosition + 24, 38, 20),
                         GTextOverflowModeFill,
                         GTextAlignmentCenter,
                         NULL);
    } else {
      snprintf(tempString, sizeof(tempString), " %d", currentTemp);

      graphics_draw_text(ctx,
                         tempString,
                         currentSidebarFont,
                         GRect(xPosition - 5 + SidebarWidgets_xOffset, yPosition + 20, 35, 20),
                         GTextOverflowModeFill,
                         GTextAlignmentCenter,
                         NULL);
    }
  } else {
    // if the weather data isn't set, draw a loading indication
    graphics_draw_text(ctx,
                       "...",
                       currentSidebarFont,
                       GRect(xPosition - 5 + SidebarWidgets_xOffset, yPosition, 38, 20),
                       GTextOverflowModeFill,
                       GTextAlignmentCenter,
                       NULL);
  }
}

/***** Bluetooth Disconnection Widget *****/

int BTDisconnect_getHeight() {
  return 22;
}

void BTDisconnect_draw(GContext* ctx, int xPosition, int yPosition) {
  if(disconnectImage) {
    gdraw_command_image_recolor(disconnectImage, globalSettings.iconFillColor, globalSettings.iconStrokeColor);


    gdraw_command_image_draw(ctx, disconnectImage, GPoint(xPosition + 3 + SidebarWidgets_xOffset, yPosition));
  }
}

/***** Week Number Widget *****/

int WeekNumber_getHeight() {
  if(SidebarWidgets_fixedHeight) {
    return FIXED_WIDGET_HEIGHT;
  } else {
    return (globalSettings.useLargeFonts) ? 31 : 26;
  }
}

void WeekNumber_draw(GContext* ctx, int xPosition, int yPosition) {
  graphics_context_set_text_color(ctx, globalSettings.sidebarTextColor);

  int yTextPosition = SidebarWidgets_fixedHeight ? yPosition + 6 : yPosition - 4;
  yTextPosition = globalSettings.useLargeFonts ? yTextPosition - 2 : yTextPosition;

  // note that it draws "above" the y position to correct for
  // the vertical padding
  graphics_draw_text(ctx,
                     wordForWeek[globalSettings.languageId],
                     currentSidebarSmallFont,
                     GRect(xPosition - 4 + SidebarWidgets_xOffset, yTextPosition, 38, 20),
                     GTextOverflowModeFill,
                     GTextAlignmentCenter,
                     NULL);

  yTextPosition = SidebarWidgets_fixedHeight ? yPosition + 15 : yPosition;
  yTextPosition = globalSettings.useLargeFonts ? yTextPosition + 6 : yTextPosition + 9;

  graphics_draw_text(ctx,
                     currentWeekNum,
                     currentSidebarFont,
                     GRect(xPosition + SidebarWidgets_xOffset, yTextPosition, 30, 20),
                     GTextOverflowModeFill,
                     GTextAlignmentCenter,
                     NULL);
}

/***** Seconds Widget *****/

int Seconds_getHeight() {
  return 14;
}

void Seconds_draw(GContext* ctx, int xPosition, int yPosition) {
  graphics_context_set_text_color(ctx, globalSettings.sidebarTextColor);

  graphics_draw_text(ctx,
                     currentSecondsNum,
                     lgSidebarFont,
                     GRect(xPosition + SidebarWidgets_xOffset, yPosition - 10, 30, 20),
                     GTextOverflowModeFill,
                     GTextAlignmentCenter,
                     NULL);
}

/***** Weather Forecast Widget *****/

int WeatherForecast_getHeight() {
  if(globalSettings.useLargeFonts) {
    return 63;
  } else {
    return 60;
  }
}

void WeatherForecast_draw(GContext* ctx, int xPosition, int yPosition) {
  graphics_context_set_text_color(ctx, globalSettings.sidebarTextColor);

  if(Weather_forecastWeatherIcon) {
    gdraw_command_image_recolor(Weather_forecastWeatherIcon, globalSettings.iconFillColor, globalSettings.iconStrokeColor);

    gdraw_command_image_draw(ctx, Weather_forecastWeatherIcon, GPoint(xPosition + 3 + SidebarWidgets_xOffset, yPosition));
  }

  // draw weather data only if it has been set
  if(Weather_weatherForecast.highTemp != INT32_MIN) {

    int highTemp = Weather_weatherForecast.highTemp;
    int lowTemp  = Weather_weatherForecast.lowTemp;

    if(!globalSettings.useMetric) {
      highTemp = roundf(highTemp * 1.8f + 32);
      lowTemp  = roundf(lowTemp * 1.8f + 32);
    }

    char tempString[8];

    graphics_context_set_fill_color(ctx, globalSettings.sidebarTextColor);

    // in large font mode, omit the degree symbol and move the text
    if(!globalSettings.useLargeFonts) {
      snprintf(tempString, sizeof(tempString), " %d°", highTemp);

      graphics_draw_text(ctx,
                         tempString,
                         currentSidebarFont,
                         GRect(xPosition - 5 + SidebarWidgets_xOffset, yPosition + 24, 38, 20),
                         GTextOverflowModeFill,
                         GTextAlignmentCenter,
                         NULL);

      graphics_fill_rect(ctx, GRect(3 + SidebarWidgets_xOffset, 8 + yPosition + 37, 24, 1), 0, GCornerNone);

      snprintf(tempString, sizeof(tempString), " %d°", lowTemp);

      graphics_draw_text(ctx,
                         tempString,
                         currentSidebarFont,
                         GRect(xPosition - 5 + SidebarWidgets_xOffset, yPosition + 42, 38, 20),
                         GTextOverflowModeFill,
                         GTextAlignmentCenter,
                         NULL);
    } else {
      snprintf(tempString, sizeof(tempString), "%d", highTemp);

      graphics_draw_text(ctx,
                         tempString,
                         currentSidebarFont,
                         GRect(xPosition + SidebarWidgets_xOffset, yPosition + 20, 30, 20),
                         GTextOverflowModeFill,
                         GTextAlignmentCenter,
                         NULL);

      graphics_fill_rect(ctx, GRect(3 + SidebarWidgets_xOffset, 8 + yPosition + 38, 24, 1), 0, GCornerNone);

      snprintf(tempString, sizeof(tempString), "%d", lowTemp);

      graphics_draw_text(ctx,
                         tempString,
                         currentSidebarFont,
                         GRect(xPosition + SidebarWidgets_xOffset, yPosition + 39, 30, 20),
                         GTextOverflowModeFill,
                         GTextAlignmentCenter,
                         NULL);
    }
  } else {
    // if the weather data isn't set, draw a loading indication
    graphics_draw_text(ctx,
                       "...",
                       currentSidebarFont,
                       GRect(xPosition - 5 + SidebarWidgets_xOffset, yPosition, 38, 20),
                       GTextOverflowModeFill,
                       GTextAlignmentCenter,
                       NULL);
  }
}

/***** Alternate Time Zone Widget *****/

int AltTime_getHeight() {
  if(SidebarWidgets_fixedHeight) {
    return FIXED_WIDGET_HEIGHT;
  } else {
    return (globalSettings.useLargeFonts) ? 31 : 26;
  }
}

void AltTime_draw(GContext* ctx, int xPosition, int yPosition) {
  graphics_context_set_text_color(ctx, globalSettings.sidebarTextColor);

  int yMod = SidebarWidgets_fixedHeight ? 6 : - 5;
  yMod = globalSettings.useLargeFonts ? yMod - 2 : yMod;

  graphics_draw_text(ctx,
                     globalSettings.altclockName,
                     currentSidebarSmallFont,
                     GRect(xPosition + SidebarWidgets_xOffset, yPosition + yMod, 30, 20),
                     GTextOverflowModeFill,
                     GTextAlignmentCenter,
                     NULL);

  yMod = SidebarWidgets_fixedHeight ? 16 : 0;
  yMod = (globalSettings.useLargeFonts) ? yMod + 5 : yMod + 8;

  graphics_draw_text(ctx,
                     altClock,
                     currentSidebarFont,
                     GRect(xPosition - 1 + SidebarWidgets_xOffset, yPosition + yMod, 30, 20),
                     GTextOverflowModeFill,
                     GTextAlignmentCenter,
                     NULL);
}

/***** Health Widget *****/

#ifdef PBL_HEALTH
int Health_getHeight() {
  if(Health_isUserSleeping()) {
    return 44;
  } else {
    if(SidebarWidgets_fixedHeight) {
      return FIXED_WIDGET_HEIGHT;
    } else {
      return 32;
    }
  }
}

void Health_draw(GContext* ctx, int xPosition, int yPosition) {
  // check if we're showing the sleep data or step data

  if(Health_isUserSleeping()) {
    Sleep_draw(ctx, xPosition, yPosition);
  } else {
    Steps_draw(ctx, xPosition, yPosition);
  }
}

void Sleep_draw(GContext* ctx, int xPosition, int yPosition) {
  if(sleepImage) {
    gdraw_command_image_recolor(sleepImage, globalSettings.iconFillColor, globalSettings.iconStrokeColor);
    gdraw_command_image_draw(ctx, sleepImage, GPoint(xPosition + 3 + SidebarWidgets_xOffset, yPosition - 7));
  }

  // get sleep in seconds
  HealthValue sleep_seconds = globalSettings.healthUseRestfulSleep ? Health_getRestfulSleepSeconds() : Health_getSleepSeconds();

  // convert to hours/minutes
  int sleep_minutes = sleep_seconds / 60;
  int sleep_hours   = sleep_minutes / 60;

  // find minutes remainder
  sleep_minutes %= 60;

  char sleep_text[4];

  snprintf(sleep_text, sizeof(sleep_text), "%ih", sleep_hours);

  graphics_context_set_text_color(ctx, globalSettings.sidebarTextColor);
  graphics_draw_text(ctx,
                     sleep_text,
                     mdSidebarFont,
                     GRect(-2 + SidebarWidgets_xOffset, yPosition + 14, 34, 20),
                     GTextOverflowModeFill,
                     GTextAlignmentCenter,
                     NULL);

  snprintf(sleep_text, sizeof(sleep_text), "%im", sleep_minutes);

  graphics_draw_text(ctx,
                     sleep_text,
                     smSidebarFont,
                     GRect(xPosition - 2 + SidebarWidgets_xOffset, yPosition + 30, 34, 20),
                     GTextOverflowModeFill,
                     GTextAlignmentCenter,
                     NULL);

}

void Steps_draw(GContext* ctx, int xPosition, int yPosition) {
  if(stepsImage) {
    int yIconPosition = SidebarWidgets_fixedHeight ? yPosition + 2 : yPosition - 7;

    gdraw_command_image_recolor(stepsImage, globalSettings.iconFillColor, globalSettings.iconStrokeColor);
    gdraw_command_image_draw(ctx, stepsImage, GPoint(xPosition + 3 + SidebarWidgets_xOffset, yIconPosition));
  }

  char steps_text[8];
  bool use_small_font = false;

  if(globalSettings.healthUseDistance) {
    HealthValue distance = Health_getDistanceWalked();
    MeasurementSystem unit_system = health_service_get_measurement_system_for_display(HealthMetricWalkedDistanceMeters);

    // format distance string
    if(unit_system == MeasurementSystemMetric) {
      if(distance < 100) {
        snprintf(steps_text, sizeof(steps_text), "%lim", distance);
      } else if(distance < 1000) {
        distance /= 100; // convert to tenths of km
        snprintf(steps_text, sizeof(steps_text), ".%likm", distance);
      } else {
        distance /= 1000; // convert to km

        if(distance > 9) {
          use_small_font = true;
        }

        snprintf(steps_text, sizeof(steps_text), "%likm", distance);
      }
    } else {
      int miles_tenths = distance * 10 / 1609 % 10;
      int miles_whole  = (int)roundf(distance / 1609.0f);

      if(miles_whole > 0) {
        snprintf(steps_text, sizeof(steps_text), "%imi", miles_whole);
      } else {
        snprintf(steps_text, sizeof(steps_text), "%c%imi", globalSettings.decimalSeparator, miles_tenths);
      }
    }
  } else {
    HealthValue steps = Health_getSteps();

    // format step string
    if(steps < 1000) {
      snprintf(steps_text, sizeof(steps_text), "%li", steps);
    } else {
      int steps_thousands = steps / 1000;
      int steps_hundreds  = steps / 100 % 10;

      if (steps < 10000) {
        snprintf(steps_text, sizeof(steps_text), "%i%c%ik", steps_thousands, globalSettings.decimalSeparator, steps_hundreds);
      } else {
        snprintf(steps_text, sizeof(steps_text), "%ik", steps_thousands);
      }
    }
  }

  graphics_context_set_text_color(ctx, globalSettings.sidebarTextColor);

  int yTextPosition = yPosition;

  if(SidebarWidgets_fixedHeight) {
    if(globalSettings.useLargeFonts) {
      yTextPosition += 26;
    } else {
      yTextPosition += 24;
    }
  } else {
    yTextPosition += 13;
  }

  graphics_draw_text(ctx,
                     steps_text,
                     (use_small_font) ? smSidebarFont : mdSidebarFont,
                     GRect(xPosition - 2 + SidebarWidgets_xOffset, yTextPosition, 35, 20),
                     GTextOverflowModeFill,
                     GTextAlignmentCenter,
                     NULL);
}

int HeartRate_getHeight() {
  if(SidebarWidgets_fixedHeight) {
    return FIXED_WIDGET_HEIGHT;
  } else if(globalSettings.useLargeFonts) {
    return 40;
  } else {
    return 38;
  }
}

void HeartRate_draw(GContext* ctx, int xPosition, int yPosition) {
  if(heartImage) {
    int yIconPosition = SidebarWidgets_fixedHeight ? yPosition + 3 : yPosition;

    gdraw_command_image_recolor(heartImage, globalSettings.iconFillColor, globalSettings.iconStrokeColor);
    gdraw_command_image_draw(ctx, heartImage, GPoint(xPosition + 3 + SidebarWidgets_xOffset, yIconPosition));
  }

  int yOffset = globalSettings.useLargeFonts ? 17 : 20;

  if(SidebarWidgets_fixedHeight) {
    yOffset += 4;
  }

  HealthValue heart_rate = Health_getHeartRate();
  char heart_rate_text[8];

  snprintf(heart_rate_text, sizeof(heart_rate_text), "%li", heart_rate);

  graphics_context_set_text_color(ctx, globalSettings.sidebarTextColor);
  graphics_draw_text(ctx,
                     heart_rate_text,
                     currentSidebarFont,
                     GRect(xPosition - 5 + SidebarWidgets_xOffset, yPosition + yOffset, 38, 20),
                     GTextOverflowModeFill,
                     GTextAlignmentCenter,
                     NULL);
}

#endif

/***** Beats (Swatch Internet Time) widget *****/

int Beats_getHeight() {
  if(SidebarWidgets_fixedHeight) {
    return FIXED_WIDGET_HEIGHT;
  } else {
    return (globalSettings.useLargeFonts) ? 31 : 26;
  }
}

void Beats_draw(GContext* ctx, int xPosition, int yPosition) {
  graphics_context_set_text_color(ctx, globalSettings.sidebarTextColor);

  int yMod = SidebarWidgets_fixedHeight ? 6 : - 5;
  yMod = globalSettings.useLargeFonts ? yMod - 2 : yMod;
  graphics_draw_text(ctx,
                     "@",
                     currentSidebarSmallFont,
                     GRect(xPosition + SidebarWidgets_xOffset, yPosition + yMod, 30, 20),
                     GTextOverflowModeFill,
                     GTextAlignmentCenter,
                     NULL);

  yMod = SidebarWidgets_fixedHeight ? 16 : 0;
  yMod = (globalSettings.useLargeFonts) ? yMod + 5 : yMod + 8;

  graphics_draw_text(ctx,
                     currentBeats,
                     currentSidebarFont,
                     GRect(xPosition + SidebarWidgets_xOffset, yPosition + yMod, 30, 20),
                     GTextOverflowModeFill,
                     GTextAlignmentCenter,
                     NULL);
}
