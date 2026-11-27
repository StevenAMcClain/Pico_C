// File: gatt.h

#ifndef GATT_H
#define GATT_H


typedef enum
{
    GATT_EXP_1         =  0,   // 1
    GATT_EXP_DECI      = -1,   // 0.1
    GATT_EXP_CENTI     = -2,   // 0.01
    GATT_EXP_MILLI     = -3,   // 0.001
    GATT_EXP_MICRO     = -6,   // 0.000001

    GATT_EXP_DECA      =  1,   // 10
    GATT_EXP_HECTO     =  2,   // 100
    GATT_EXP_KILO      =  3,   // 1000
    GATT_EXP_MEGA      =  6,   // 1000000

} gatt_exponent_t;


typedef enum
{
    GATT_FMT_BOOLEAN   = 0x01,

    GATT_FMT_UINT8     = 0x04,
    GATT_FMT_UINT16    = 0x06,
    GATT_FMT_UINT32    = 0x08,
    GATT_FMT_UINT64    = 0x0A,

    GATT_FMT_SINT8     = 0x0C,
    GATT_FMT_SINT16    = 0x0E,
    GATT_FMT_SINT32    = 0x10,
    GATT_FMT_SINT64    = 0x12,

    GATT_FMT_FLOAT32   = 0x14,
    GATT_FMT_FLOAT64   = 0x15,

    GATT_FMT_SFLOAT    = 0x16,  // IEEE-11073 16-bit
    GATT_FMT_FLOAT11073= 0x17,  // IEEE-11073 32-bit

    GATT_FMT_UTF8S     = 0x19,
    GATT_FMT_UTF16S    = 0x1A,

    GATT_FMT_STRUCT    = 0x1B

} gatt_format_t;


typedef enum
{
    GATT_NAMESPACE_BLUETOOTH_SIG = 0x01,

    GATT_NAMESPACE_RESERVED      = 0x00

} gatt_namespace_t;


typedef enum
{
    GATT_UNIT_UNITLESS             = 0x2700,

    GATT_UNIT_METRE                = 0x2701,
    GATT_UNIT_KILOGRAM             = 0x2702,
    GATT_UNIT_SECOND               = 0x2703,
    GATT_UNIT_AMPERE               = 0x2704,
    GATT_UNIT_KELVIN               = 0x2705,
    GATT_UNIT_MOLE                 = 0x2706,
    GATT_UNIT_CANDELA              = 0x2707,

    GATT_UNIT_SQUARE_METRE         = 0x2710,
    GATT_UNIT_CUBIC_METRE          = 0x2711,
    GATT_UNIT_METRES_PER_SECOND    = 0x2712,
    GATT_UNIT_METRES_PER_SEC2      = 0x2713,

    GATT_UNIT_RECIPROCAL_METRE     = 0x2714,
    GATT_UNIT_DENSITY              = 0x2715,

    GATT_UNIT_PASCAL               = 0x2724,
    GATT_UNIT_ENERGY_JOULE         = 0x2725,
    GATT_UNIT_POWER_WATT           = 0x2726,

    GATT_UNIT_ELECTRIC_CHARGE      = 0x2727,
    GATT_UNIT_TEMP_CELSIUS         = 0x2728,
    GATT_UNIT_TEMP_FAHRENHEIT      = 0x27AC,

    GATT_UNIT_FREQUENCY_HERTZ      = 0x2722,

    GATT_UNIT_FORCE_NEWTON         = 0x2723,

    GATT_UNIT_VOLT                 = 0x272F,
    GATT_UNIT_AMP                  = 0x2730,
    GATT_UNIT_RESISTANCE_OHM       = 0x2731,
    GATT_UNIT_CONDUCTANCE_SIEMENS  = 0x2733,

    GATT_UNIT_CAPACITANCE_FARAD    = 0x2735,
    GATT_UNIT_FLUX_WEBER           = 0x2736,
    GATT_UNIT_FIELD_TESLA          = 0x2737,
    GATT_UNIT_INDUCTANCE_HENRY     = 0x2738,

    GATT_UNIT_LUMEN                = 0x2739,
    GATT_UNIT_LUX                  = 0x273A,

    GATT_UNIT_RADIOACTIVITY_BQ     = 0x273D,

    GATT_UNIT_GRAY                 = 0x273E,
    GATT_UNIT_SIEVERT              = 0x273F,

    GATT_UNIT_CATALYTIC_ACTIVITY   = 0x2740,

    GATT_UNIT_PRESSURE_MMHG        = 0x2781,

    GATT_UNIT_TIME_MINUTE          = 0x2760,
    GATT_UNIT_TIME_HOUR            = 0x2761,
    GATT_UNIT_TIME_DAY             = 0x2762,

    GATT_UNIT_PERCENTAGE           = 0x27AD,

} gatt_unit_t;

#define GATT_UNIT(x) (x >> 8) & 0xFF, (x & 0xFF)


#endif // GATT_H


// End of file: gatt.h
