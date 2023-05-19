#include "config_paser.hpp"

void config_paser()
{
    IniFile * ini = new IniFile();
    ini->load("sensor_config.ini");
    ini->show();
}