/* Header */
#include "lang/lang.h"

/* Language table */
static const char* const raw_translations[LANGUAGE_ID_COUNT][STRING_ID_COUNT] =
{
    [LANGUAGE_ID_DE] =
    {
        [STRING_ID_MENU_FILE] = "Datei",
        [STRING_ID_MENU_EDIT] = "Bearbeiten",
        [STRING_ID_MENU_VIEW] = "Ansicht",
        [STRING_ID_MENU_CONSOLE] = "Konsole",
        [STRING_ID_MENU_GRAPH] = "Diagramm",
        [STRING_ID_MENU_HELP] = "Hilfe",
        [STRING_ID_FILE_NEW] = "Neu",
        [STRING_ID_FILE_OPEN] = "Öffnen",
        [STRING_ID_FILE_SAVE] = "Speichern",
        [STRING_ID_FILE_EXIT] = "Beenden",
        [STRING_ID_FILE_OPEN_GRAPH] = "Graph öffnen...",
        [STRING_ID_FILE_OPEN_CONFIG] = "Regler Konfig. öffnen...",
        [STRING_ID_FILE_OPEN_CONSOLE] = "Konsole öffnen...",
        [STRING_ID_FILE_SAVE_GRAPH_PNG] = "Graph speichern (.png)...",
        [STRING_ID_FILE_SAVE_GRAPH_DATA] = "Graph speichern (.csv)...",
        [STRING_ID_FILE_SAVE_CONFIG] = "Regler Einstellung speichern...",
        [STRING_ID_FILE_SAVE_CONSOLE] = "Konsolenausgabe speichern...",
        [STRING_ID_VIEW_STYLE] = "Thema",
        [STRING_ID_VIEW_LANG] = "Sprache",
        [STRING_ID_VIEW_STYLE_DARK] = "Dunkel",
        [STRING_ID_VIEW_STYLE_LIGHT] = "Hell",
        [STRING_ID_VIEW_LANG_DE] = "Deutsch",
        [STRING_ID_VIEW_LANG_EN] = "Englisch",
        [STRING_ID_CONNECTED] = "Verbunden",
        [STRING_ID_DISCONNECTED] = "Getrennt",
        [STRING_ID_AUTO_FOLLOW] = "Auto-Follow",
        [STRING_ID_CLEAR] = "Leeren",
        [STRING_ID_DATA_POINTS] = "Datenpunkte",
        [STRING_ID_COM] = "COM",
        [STRING_ID_RESET] = "Zurücksetzen",
        [STRING_ID_BAUD] = "Baud",
        [STRING_ID_CONNECT] = "Verbinden",
        [STRING_ID_DISCONNECT] = "Trennen",
		[STRING_ID_OK] = "OK",
		[STRING_ID_DEVICE] = "Gerät",
		[STRING_ID_FLASH] = "Flashen",
		[STRING_ID_CANCEL] = "Abbrechen",
        [STRING_ID_FLASH_FIRMWARE] = "Firmware flashen...",
		[STRING_ID_FIRMWARE_UPDATER] = "Firmware Updater",
        [STRING_ID_FLASH_WARNING] = "Warnung: Während des Flashens werden alle aktiven Messungen unterbrochen. Bitte trennen Sie das Gerät nicht während diesem Vorgang!",
		[STRING_ID_FIRMWARE_FILE] = "Firmware-Datei:",
		[STRING_ID_PROGRESS] = "Fortschritt:",
		[STRING_ID_ENABLE_MANUAL_CONTROL] = "Manuelle Steuerung aktivieren:",
        [STRING_ID_ENABLE_TWO_POINT_CONTROL] = "Zweipunktregler aktivieren:",
        [STRING_ID_ENABLE_AUTO_CONTROL] = "Regelung (PID) aktivieren:",
        [STRING_ID_P_TERM] = "P-Anteil",
        [STRING_ID_I_TERM] = "I-Anteil",
        [STRING_ID_D_TERM] = "D-Anteil",
        [STRING_ID_LABEL_PWM] = "PWM (0-100%):",
        [STRING_ID_LABEL_SET_POINT] = "Solltemperatur (°C):",
        [STRING_ID_LABEL_HYSTERESE] = "Schaltdifferenz (°C):",
        [STRING_ID_P_VARIABLE] = "Kp:",
        [STRING_ID_I_VARIABLE] = "Ki:",
        [STRING_ID_D_VARIABLE] = "Kd:",
        [STRING_ID_EXPORT_GRAPH_DIALOG_TITLE] = "Graph exportieren",
        [STRING_ID_PNG_FILES] = "PNG Bilder",
        [STRING_ID_CSV_FILES] = "CSV UTF-8",
        [STRING_ID_CSV_FILENAME] = "messdaten.csv",
        [STRING_ID_PNG_FILENAME] = "graph.png",
        [STRING_ID_EXPORT_CONSOLE_DIALOG_TITLE] = "Konsole exportieren",
        [STRING_ID_TXT_FILES] = "Textdokumente",
        [STRING_ID_TXT_FILENAME] = "konsole.txt",
        [STRING_ID_ENABLE_SAMPLING] = "Mittelung aktivieren",
        [STRING_ID_DISABLE_SAMPLING] = "Mittelung deaktivieren",
        [STRING_ID_EXPORT_CONFIG_DIALOG_TITLE] = "Reglerkonfiguration speichern",
        [STRING_ID_CFG_FILES] = "Konfigurationsdateien",
        [STRING_ID_CFG_FILENAME] = "regler_konfiguration.cfg",
        [STRING_ID_IMPORT] = "Importieren...",
        [STRING_ID_IMPORT_WARNING] = "Warnung! Importieren...",
        [STRING_ID_IMPORT_WARNING_TEXT] = "Achtung: Beim Importieren wird die aktuelle Verbindung zum Sensor getrennt.",
        [STRING_ID_IMPORT_DATA_DIALOG_TITLE] = "Messdaten importieren",
        [STRING_ID_IMPORT_CONFIG_DIALOG_TITLE] = "Reglerkonfiguration importieren",
        [STRING_ID_IMPORT_CONSOLE_DIALOG_TITLE] = "Konsole importieren",
        [STRING_ID_AUTO_CONNECT] = "Automatisch verbinden"
    },
    [LANGUAGE_ID_EN] =
    {
        [STRING_ID_MENU_FILE] = "File",
        [STRING_ID_MENU_EDIT] = "Edit",
        [STRING_ID_MENU_VIEW] = "View",
        [STRING_ID_MENU_CONSOLE] = "Console",
        [STRING_ID_MENU_GRAPH] = "Graph",
        [STRING_ID_MENU_HELP] = "Help",
        [STRING_ID_FILE_NEW] = "New",
        [STRING_ID_FILE_OPEN] = "Open",
        [STRING_ID_FILE_SAVE] = "Save",
        [STRING_ID_FILE_EXIT] = "Exit",
        [STRING_ID_FILE_OPEN_GRAPH] = "Open graph...",
        [STRING_ID_FILE_OPEN_CONFIG] = "Open controller config...",
        [STRING_ID_FILE_OPEN_CONSOLE] = "Open console data...",
        [STRING_ID_FILE_SAVE_GRAPH_PNG] = "Save graph (.png)...",
        [STRING_ID_FILE_SAVE_GRAPH_DATA] = "Save graph (.csv)...",
        [STRING_ID_FILE_SAVE_CONFIG] = "Save control settings...",
        [STRING_ID_FILE_SAVE_CONSOLE] = "Save console output...",
        [STRING_ID_VIEW_STYLE] = "Theme",
        [STRING_ID_VIEW_LANG] = "Language",
        [STRING_ID_VIEW_STYLE_DARK] = "Dark",
        [STRING_ID_VIEW_STYLE_LIGHT] = "Light",
        [STRING_ID_VIEW_LANG_DE] = "German",
        [STRING_ID_VIEW_LANG_EN] = "English",
        [STRING_ID_CONNECTED] = "Connected",
        [STRING_ID_DISCONNECTED] = "Disconnected",
        [STRING_ID_AUTO_FOLLOW] = "Auto-Follow",
        [STRING_ID_CLEAR] = "Clear",
        [STRING_ID_DATA_POINTS] = "Data points",
        [STRING_ID_COM] = "COM",
        [STRING_ID_RESET] = "Reset",
        [STRING_ID_BAUD] = "Baud",
        [STRING_ID_CONNECT] = "Connect",
        [STRING_ID_DISCONNECT] = "Disconnect",
		[STRING_ID_OK] = "OK",
        [STRING_ID_DEVICE] = "Device",
        [STRING_ID_FLASH] = "Flash",
        [STRING_ID_CANCEL] = "Cancel",
        [STRING_ID_FLASH_FIRMWARE] = "Flash Firmware...",
        [STRING_ID_FIRMWARE_UPDATER] = "Firmware Updater",
		[STRING_ID_FLASH_WARNING] = "Warning: During flashing, all active measurements will be interrupted. Please do not disconnect the device during this process!",
		[STRING_ID_FIRMWARE_FILE] = "Firmware file:",
		[STRING_ID_PROGRESS] = "Progress:",
		[STRING_ID_ENABLE_MANUAL_CONTROL] = "Enable manual control:",
        [STRING_ID_ENABLE_TWO_POINT_CONTROL] = "Enable two-point control:",
        [STRING_ID_ENABLE_AUTO_CONTROL] = "Enable auto control:",
        [STRING_ID_P_TERM] = "P Term",
        [STRING_ID_I_TERM] = "I Term",
        [STRING_ID_D_TERM] = "D Term",
        [STRING_ID_LABEL_PWM] = "PWM (0-100%):",
        [STRING_ID_LABEL_SET_POINT] = "Target temperature (°C):",
        [STRING_ID_LABEL_HYSTERESE] = "Hysterese (°C):",
        [STRING_ID_P_VARIABLE] = "Kp:",
        [STRING_ID_I_VARIABLE] = "Ki:",
        [STRING_ID_D_VARIABLE] = "Kd:",
        [STRING_ID_EXPORT_GRAPH_DIALOG_TITLE] = "Export Graph",
        [STRING_ID_PNG_FILES] = "PNG Images",
        [STRING_ID_CSV_FILES] = "CSV UTF-8",
        [STRING_ID_CSV_FILENAME] = "measurement_data.csv",
        [STRING_ID_PNG_FILENAME] = "graph.png",
        [STRING_ID_EXPORT_CONSOLE_DIALOG_TITLE] = "Export console",
        [STRING_ID_TXT_FILES] = "Text files",
        [STRING_ID_TXT_FILENAME] = "console.txt",
        [STRING_ID_ENABLE_SAMPLING] = "Enable sampling",
        [STRING_ID_DISABLE_SAMPLING] = "Disable sampling",
        [STRING_ID_EXPORT_CONFIG_DIALOG_TITLE] = "Save Controller Configuration",
        [STRING_ID_CFG_FILES] = "Configuration Files",
        [STRING_ID_CFG_FILENAME] = "controller_configuration.cfg",
        [STRING_ID_IMPORT] = "Import...",
        [STRING_ID_IMPORT_WARNING] = "Warning! Import...",
        [STRING_ID_IMPORT_WARNING_TEXT] = "Warning: Importing will disconnect the current sensor connection.",
        [STRING_ID_IMPORT_DATA_DIALOG_TITLE] = "Import measurement data",
        [STRING_ID_IMPORT_CONFIG_DIALOG_TITLE] = "Import controller configuration",
        [STRING_ID_IMPORT_CONSOLE_DIALOG_TITLE] = "Import console",
        [STRING_ID_AUTO_CONNECT] = "Auto Connect"
     },
};

/* Static local variables */
static const lang_string_t fallback_string = { .chars = "???", .length = 3 };
static lang_string_t string_cache[STRING_ID_COUNT];
static language_id_t current_language = LANGUAGE_ID_DE;

/* Static function definitions */
static void rebuild_cache(void)
{
    /* Iterate through all strings & validate them & check the size */
    for (int string_index = 0; string_index < STRING_ID_COUNT; string_index++)
    {
        const char* raw = raw_translations[current_language][string_index];

        /* If string is empty, fill it with fallback */
        if (raw == NULL)
        {
            string_cache[string_index] = fallback_string;
            continue;
        }

        /* Copy pointer to string & calculate length */
        string_cache[string_index].chars = raw;
        string_cache[string_index].length = (int32_t)strlen(raw);
    }
    return;
}

/* Function definitions */
void lang_init(const language_id_t initial_language)
{
    /* Safety check */
    if ((initial_language < 0) || (initial_language >= LANGUAGE_ID_COUNT))
    {
        current_language = LANGUAGE_ID_DE;
    }

    /* Set language & rebuild it */
    current_language = initial_language;
    rebuild_cache();
    return;
}

void lang_set_language(const language_id_t language)
{
    lang_init(language);
    return;
}

language_id_t lang_get_current_language(void)
{
    return current_language;
}

const lang_string_t* lang_get(string_id_t id)
{
    /* Safety check */
    if ((id < 0) || (id >= STRING_ID_COUNT))
    {
        return &fallback_string;
    }

    return &string_cache[id];
}

const Clay_String get_label(const string_id_t id)
{
    const lang_string_t* str = lang_get(id);
    Clay_String label = { .length = str->length, .chars = str->chars };
    return label;
}