try:
    # Python 2.7
    from Tkinter import *
    import ttk
    import tkMessageBox
    import tkFileDialog
    import tkColorChooser
except ImportError:
    # Python 3
    from tkinter import *
    from tkinter import ttk
    from tkinter import messagebox as tkMessageBox
    from tkinter import filedialog as tkFileDialog
    from tkinter import colorchooser as tkColorChooser

import PluginOptionsJsonBuilder

bullet_char = u'\u2022'

OPTION_TRANSTIME  = "transTime"
OPTION_DELAYTIME  = "delayTime"
OPTION_WINDOWSIZE = "nColorsPerFrame"
OPTION_LINDIR     = "linDirection"
OPTION_RADDIR     = "radDirection"
OPTION_ROTDIR     = "rotDirection"
OPTION_LOOP       = "loop"

class PluginOptionsGUI:
    def __init__(self, root):
        self.root = root
        self.pluginOptionTypes = [OPTION_TRANSTIME, OPTION_DELAYTIME, OPTION_WINDOWSIZE, OPTION_LINDIR, OPTION_RADDIR, OPTION_ROTDIR, OPTION_LOOP]
        self.linDirectionOptions = ["left", "right", "up", "down"]
        self.radDirectionOptions = ["in", "out"]
        self.rotDirectionOptions = ["cw", "ccw"]
        self.boolOptions = ["False", "True"]
        self.pluginOptionRows = []

    def set_plugin_dir(self, plugin_dir):
        self.plugin_dir = plugin_dir
        self.header_path = self.plugin_dir + "/inc/PluginOptions.h"

    def update_plugin_dir(self, plugin_dir):
        self.set_plugin_dir(plugin_dir)
        self.clear_plugin_options()
        self.load_options_from_header()

    def has_plugin_options(self):
        if len(self.pluginOptionRows):
            return True
        return False

    def create_plugin_frame(self):
        self.mainPluginFrame = ttk.Frame(self.root)
        self.mainPluginFrame.grid(column=0, row=1, sticky=(N, W, E, S), padx=(10, 10))
        self.mainPluginFrame.columnconfigure(0, weight=1)
        self.mainPluginFrame.rowconfigure(0, weight=1)

        self.emptyStateButton = None
        self.showingHeader = False
        self.headerLabel = None
        self.addButton = None
        self.generateButton = None

        self.pluginOptionBaseRow = 1

        self.load_options_from_header()

    def toggle_empty_state(self, show, addRow=True):
        if (show):
            self.root.minsize(width=0, height=260)
            self.root.maxsize(width=1200, height=260)
            self.emptyStateButton = ttk.Button(self.mainPluginFrame, text="Add Plugin Options", command=lambda: self.toggle_empty_state(False))
            self.emptyStateButton.grid(column=0, row=0, sticky=(N,W,E,S))
            if self.showingHeader:
                self.headerLabel.destroy()
                self.addButton.destroy()
                self.generateButton.destroy()
        else:
            self.root.minsize(width=0, height=380)
            self.root.maxsize(width=1200, height=380)
            self.showingHeader = True
            self.headerLabel = ttk.Label(self.mainPluginFrame, text="Plugin Options")
            self.headerLabel.grid(column=0, row=0, sticky=(N,W), pady=(0, 15))
            self.addButton = ttk.Button(self.mainPluginFrame, text='Add Plugin Option', command=self.add_plugin_option)
            self.addButton.grid(column=1, row=0, sticky=(N,E))
            self.generateButton = ttk.Button(self.mainPluginFrame, text='Generate Header', command=self.generate_plugin_options_header)
            self.generateButton.grid(column=2, row=0, sticky=(N,E))
            if self.emptyStateButton:
                if addRow:
                    self.add_plugin_option()
                self.emptyStateButton.destroy()

    def load_options_from_header(self):
        options = PluginOptionsJsonBuilder.load_from_header_file(self.header_path)
        if len(options) > 0:
            self.toggle_empty_state(False, False)
            self.load_options(options)
        else:
            self.toggle_empty_state(True, False)

    def load_options(self, options):
        for option in options:
            if self.plugin_option_is_type_int(option["name"]):
                pluginOptionRow = self.create_plugin_option_row(option["name"], defaultVal=option["defaultValue"], minVal=option["minValue"], maxVal=option["maxValue"])
            elif option["name"] == OPTION_LINDIR:
                enabledOptions = [0, 0, 0, 0]
                for enabledOption in option["strings"]:
                    index = self.linDirectionOptions.index(enabledOption)
                    enabledOptions[index] = 1
                pluginOptionRow = self.create_plugin_option_row(option["name"], defaultVal=option["defaultValue"], enabledOptionsVals=enabledOptions)
            elif self.plugin_option_is_type_string(option["name"]):
                pluginOptionRow = self.create_plugin_option_row(option["name"], defaultVal=option["defaultValue"])
            elif option["name"] == OPTION_LOOP:
                defaultVal = int(option["defaultValue"])
                pluginOptionRow = self.create_plugin_option_row(option["name"], defaultVal=defaultVal)
            else:
                continue
            self.pluginOptionRows.append(pluginOptionRow)

    def generate_plugin_options_header(self, show_success=True):
        if not self.validate_options():
            joinStr = "\n\n" + bullet_char + " "
            err = bullet_char + " " + joinStr.join(self.validation_error_messages)
            tkMessageBox.showerror("Error", err)
            return -1

        options = []
        for pluginOptionRow in self.pluginOptionRows:
            if pluginOptionRow["optionTypeVar"].get() == OPTION_TRANSTIME:
                options.append(PluginOptionsJsonBuilder.make_transtime(int(pluginOptionRow["optionDefaultVar"].get()), int(pluginOptionRow["optionMaxVar"].get()), int(pluginOptionRow["optionMinVar"].get())))
            elif pluginOptionRow["optionTypeVar"].get() == OPTION_DELAYTIME:
                options.append(PluginOptionsJsonBuilder.make_delaytime(int(pluginOptionRow["optionDefaultVar"].get()), int(pluginOptionRow["optionMaxVar"].get()), int(pluginOptionRow["optionMinVar"].get())))
            elif pluginOptionRow["optionTypeVar"].get() == OPTION_WINDOWSIZE:
                options.append(PluginOptionsJsonBuilder.make_ncolorsperframe(int(pluginOptionRow["optionDefaultVar"].get()), int(pluginOptionRow["optionMaxVar"].get()), int(pluginOptionRow["optionMinVar"].get())))
            elif pluginOptionRow["optionTypeVar"].get() == OPTION_LINDIR:
                enabled_options = []
                for enabledOptionVar in pluginOptionRow["enabledOptionsVars"]:
                    if enabledOptionVar.get() == 1:
                        index = pluginOptionRow["enabledOptionsVars"].index(enabledOptionVar)
                        enabled_options.append(self.linDirectionOptions[index])
                options.append(PluginOptionsJsonBuilder.make_lindirection(pluginOptionRow["optionDefaultVar"].get(), enabled_options))
            elif pluginOptionRow["optionTypeVar"].get() == OPTION_RADDIR:
                options.append(PluginOptionsJsonBuilder.make_raddirection(pluginOptionRow["optionDefaultVar"].get()))
            elif pluginOptionRow["optionTypeVar"].get() == OPTION_ROTDIR:
                options.append(PluginOptionsJsonBuilder.make_rotdirection(pluginOptionRow["optionDefaultVar"].get()))
            elif pluginOptionRow["optionTypeVar"].get() == OPTION_LOOP:
                val = True if pluginOptionRow["optionDefaultVar"].get() == "True" else False
                options.append(PluginOptionsJsonBuilder.make_loop(val))

        PluginOptionsJsonBuilder.write_options_to_header_file(options, self.header_path)
        if show_success:
            tkMessageBox.showinfo("Plugin Options", "Successfully generated PluginOptions.h")
        return 0

    def set_plugin_option(self, pluginOptionType, rowIndex=None):
        rowToDestroy = None
        for child in self.mainPluginFrame.children.values():
            if int(child.grid_info()["row"]) == rowIndex:
                rowToDestroy = child

        if rowIndex is None or rowToDestroy is None:
            return

        newPluginOptionRow = self.create_plugin_option_row(pluginOptionType, rowIndex)
        rowToDestroy.destroy()
        self.pluginOptionRows[rowIndex - self.pluginOptionBaseRow] = newPluginOptionRow

    def add_plugin_option(self):
        if len(self.pluginOptionRows) > 3:
            tkMessageBox.showerror("Error", "Cannot have more than 4 options!")
            return

        newOption = None
        for option in self.pluginOptionTypes:
            optionSelected = False
            for row in self.pluginOptionRows:
                if row["optionTypeVar"].get() == option:
                    optionSelected = True
                    break
            if optionSelected == False:
                newOption = option
                break

        pluginOptionRow = self.create_plugin_option_row(option)
        self.pluginOptionRows.append(pluginOptionRow)

    def remove_plugin_option(self, pluginOptionRow):
        if "rowFrame" in pluginOptionRow:
            pluginOptionRow["rowFrame"].destroy()
        self.pluginOptionRows.remove(pluginOptionRow)
        if len(self.pluginOptionRows) == 0:
            self.toggle_empty_state(True)

    def clear_plugin_options(self):
        for pluginOptionRow in reversed(self.pluginOptionRows):
            self.remove_plugin_option(pluginOptionRow)

    def create_plugin_option_row(self, pluginOptionType, row=-1, defaultVal=-1, minVal=-1, maxVal=-1, enabledOptionsVals=[]):
        if not self.plugin_option_is_type_int(pluginOptionType) and not self.plugin_option_is_type_string(pluginOptionType) and not self.plugin_option_is_type_bool(pluginOptionType):
            return

        optionNumber = len(self.pluginOptionRows)
        if row == -1:
            if self.pluginOptionRows:
                row = int(self.pluginOptionRows[-1]["rowFrame"].grid_info()["row"]) + 1
            else:
                row = self.pluginOptionBaseRow + optionNumber

        pluginOptionFrame = ttk.Frame(self.mainPluginFrame)
        pluginOptionFrame.grid(column=0, row=row, sticky=(N, W, E, S), columnspan=3)
        pluginOptionFrame.columnconfigure(0, weight=1, minsize=90)
        pluginOptionFrame.columnconfigure(1, weight=1)
        pluginOptionFrame.columnconfigure(2, weight=1)
        pluginOptionFrame.columnconfigure(3, weight=1)
        pluginOptionFrame.columnconfigure(4, weight=1)
        pluginOptionFrame.columnconfigure(5, weight=1)
        pluginOptionFrame.columnconfigure(6, weight=1, minsize=90)
        pluginOptionFrame.columnconfigure(7, weight=1, minsize=90)
        pluginOptionFrame.columnconfigure(8, weight=1, minsize=90)
        pluginOptionFrame.rowconfigure(0, weight=1)

        pluginOptionTypeVar = StringVar()
        pluginOptionTypeVar.set(pluginOptionType)
        optionmenu = OptionMenu(pluginOptionFrame, pluginOptionTypeVar, *self.pluginOptionTypes, command=lambda type: self.set_plugin_option(type, row))
        optionmenu.grid(column=0, row=row, sticky=(E,W))
        optionmenu.configure(width=15)
        optionmenu.configure(background="#E4E4E4")
        if self.plugin_option_is_type_int(pluginOptionType):
            newPluginOptionRow = self.create_int_plugin_option_row(pluginOptionType, pluginOptionFrame, pluginOptionTypeVar, row, defaultVal=defaultVal, minVal=minVal, maxVal=maxVal)
        elif self.plugin_option_is_type_string(pluginOptionType):
            newPluginOptionRow = self.create_string_plugin_option_row(pluginOptionType, pluginOptionFrame, pluginOptionTypeVar, row, defaultVal=defaultVal, enabledOptionsVals=enabledOptionsVals)
        elif self.plugin_option_is_type_bool(pluginOptionType):
            newPluginOptionRow = self.create_bool_plugin_option_row(pluginOptionType, pluginOptionFrame, pluginOptionTypeVar, row, defaultVal=defaultVal)
        else:
            return

        return newPluginOptionRow

    def create_int_plugin_option_row(self, pluginOptionType, pluginOptionFrame, pluginOptionTypeVar, row=-1, defaultVal=-1, minVal=-1, maxVal=-1):
        if not self.plugin_option_is_type_int(pluginOptionType):
            return

        if pluginOptionType == OPTION_TRANSTIME:
            if defaultVal == -1:
                defaultVal = 15
            if minVal == -1:
                minVal = 1
            if maxVal == -1:
                maxVal = 600
        elif pluginOptionType == OPTION_DELAYTIME:
            if defaultVal == -1:
                defaultVal = 15
            if minVal == -1:
                minVal = 0
            if maxVal == -1:
                maxVal = 600
        elif pluginOptionType == OPTION_WINDOWSIZE:
            if defaultVal == -1:
                defaultVal = 1
            if minVal == -1:
                minVal = 1
            if maxVal == -1:
                maxVal = 50

        ttk.Label(pluginOptionFrame, text='Min Value:').grid(column=1, row=row, sticky=(E))
        pluginOptionMinValue = StringVar()
        if minVal > -1:
            pluginOptionMinValue.set(minVal)
        ttk.Entry(pluginOptionFrame, width=5, textvariable=pluginOptionMinValue, justify='center').grid(column=2, row=row, sticky=(W))

        ttk.Label(pluginOptionFrame, text='Max Value:').grid(column=3, row=row, sticky=(E))
        pluginOptionMaxValue = StringVar()
        if maxVal > -1:
            pluginOptionMaxValue.set(maxVal)
        pluginOptionMaxElement = ttk.Entry(pluginOptionFrame, width=5, textvariable=pluginOptionMaxValue, justify='center').grid(column=4, row=row, sticky=(W))

        ttk.Label(pluginOptionFrame, text=' ').grid(column=5, row=row)
        ttk.Label(pluginOptionFrame, text='Default Value:').grid(column=6, row=row, sticky=(E))
        pluginOptionDefaultValue = StringVar()
        if defaultVal > -1:
            pluginOptionDefaultValue.set(defaultVal)
        ttk.Entry(pluginOptionFrame, width=5, textvariable=pluginOptionDefaultValue, justify='center').grid(column=7, row=row)

        pluginOptionRow = {
                            "rowFrame": pluginOptionFrame,
                            "optionTypeVar": pluginOptionTypeVar,
                            "optionMinVar": pluginOptionMinValue,
                            "optionMaxVar": pluginOptionMaxValue,
                            "optionDefaultVar": pluginOptionDefaultValue
                        }

        ttk.Button(pluginOptionFrame, text='Remove', command=lambda: self.remove_plugin_option(pluginOptionRow)).grid(column=8, row=row, sticky=(E))

        return pluginOptionRow

    def create_string_plugin_option_row(self, pluginOptionType, pluginOptionFrame, pluginOptionTypeVar, row=-1, defaultVal="", enabledOptionsVals=[]):
        if not self.plugin_option_is_type_string(pluginOptionType):
            return

        column = 1

        pluginOptionDefaultValue = StringVar()
        enabledOptions = []
        if pluginOptionType == OPTION_LINDIR:
            ttk.Label(pluginOptionFrame, text='Enabled:').grid(column=column, row=row, sticky=(E))
            column += 1
            defaultOptions = self.linDirectionOptions
            for value in defaultOptions:
                index = defaultOptions.index(value)
                isEnabled = IntVar()
                if len(enabledOptionsVals) == 4:
                    isEnabled.set(enabledOptionsVals[index])
                else:
                    isEnabled.set(1)
                enabledOptions.append(isEnabled)
                ttk.Checkbutton(pluginOptionFrame, variable=enabledOptions[index], text=value).grid(column=column, row=row)
                column += 1
        elif pluginOptionType == OPTION_RADDIR or pluginOptionType == OPTION_ROTDIR:
            ttk.Label(pluginOptionFrame, text='            ').grid(column=1, row=row)
            ttk.Label(pluginOptionFrame, text='            ').grid(column=2, row=row)
            ttk.Label(pluginOptionFrame, text='            ').grid(column=3, row=row)
            ttk.Label(pluginOptionFrame, text='            ').grid(column=4, row=row)
            ttk.Label(pluginOptionFrame, text='            ').grid(column=5, row=row)
            if pluginOptionType == OPTION_RADDIR:
                defaultOptions = self.radDirectionOptions
            elif pluginOptionType == OPTION_ROTDIR:
                defaultOptions = self.rotDirectionOptions
        
        if defaultVal and defaultVal in defaultOptions:
            pluginOptionDefaultValue.set(defaultVal)
        else:
            pluginOptionDefaultValue.set(defaultOptions[0])
        ttk.Label(pluginOptionFrame, text='Default Value:').grid(column=6, row=row, sticky=(E))
        column += 1
        optionmenu = OptionMenu(pluginOptionFrame, pluginOptionDefaultValue, *defaultOptions)
        optionmenu.grid(column=7, row=row)
        # Known issue with MacOS TCL where the background color of ttk is not defined and different from the Tkinter background color
        operating_sys = sys.platform
        if operating_sys == "darwin":
            optionmenu.configure(background="#E4E4E4")
        column += 1

        pluginOptionRow = {
                            "rowFrame": pluginOptionFrame,
                            "optionTypeVar": pluginOptionTypeVar,
                            "optionDefaultVar": pluginOptionDefaultValue
                        }
        if pluginOptionType == OPTION_LINDIR:
            pluginOptionRow["enabledOptionsVars"] = enabledOptions;

        ttk.Button(pluginOptionFrame, text='Remove', command=lambda: self.remove_plugin_option(pluginOptionRow)).grid(column=8, row=row, sticky=(E))
        column += 1

        return pluginOptionRow

    def create_bool_plugin_option_row(self, pluginOptionType, pluginOptionFrame, pluginOptionTypeVar, row=-1, defaultVal=1):
        if not self.plugin_option_is_type_bool(pluginOptionType):
            return

        ttk.Label(pluginOptionFrame, text='            ').grid(column=1, row=row)
        ttk.Label(pluginOptionFrame, text='            ').grid(column=2, row=row)
        ttk.Label(pluginOptionFrame, text='            ').grid(column=3, row=row)
        ttk.Label(pluginOptionFrame, text='            ').grid(column=4, row=row)
        ttk.Label(pluginOptionFrame, text='            ').grid(column=5, row=row)
        ttk.Label(pluginOptionFrame, text='Default Value:').grid(column=6, row=row, sticky=(E))
        pluginOptionDefaultValue = StringVar()
        pluginOptionDefaultValue.set(self.boolOptions[defaultVal])
        optionmenu = OptionMenu(pluginOptionFrame, pluginOptionDefaultValue, *self.boolOptions)
        optionmenu.grid(column=7, row=row, sticky=(W))
        # Known issue with MacOS TCL where the background color of ttk is not defined and different from the Tkinter background color
        operating_sys = sys.platform
        if operating_sys == "darwin":
            optionmenu.configure(background="#E4E4E4")

        pluginOptionRow = {
                            "rowFrame": pluginOptionFrame,
                            "optionTypeVar": pluginOptionTypeVar,
                            "optionDefaultVar": pluginOptionDefaultValue
                        }

        ttk.Button(pluginOptionFrame, text='Remove', command=lambda: self.remove_plugin_option(pluginOptionRow)).grid(column=8, row=row, sticky=(E))

        return pluginOptionRow

    def validate_options(self):
        self.validation_error_messages = []
        valid = True
        optionsNames = []
        if len(self.pluginOptionRows) < 1:
            self.validation_error_messages.append("No plugin options")
            valid = False
        for optionRow in self.pluginOptionRows:
            optionsNames.append(optionRow["optionTypeVar"].get())
        if len(optionsNames) > len(set(optionsNames)):
            self.validation_error_messages.append("Cannot have duplicate plugin options")
            valid = False
        
        for pluginOptionRow in self.pluginOptionRows:
            valid = self.validate_option(pluginOptionRow) and valid

        return valid

    def validate_option(self, pluginOptionRow):
        if self.plugin_option_is_type_int(pluginOptionRow["optionTypeVar"].get()):
            return self.validate_int_option(pluginOptionRow)
        elif self.plugin_option_is_type_string(pluginOptionRow["optionTypeVar"].get()):
            return self.validate_string_option(pluginOptionRow)
        elif self.plugin_option_is_type_bool(pluginOptionRow["optionTypeVar"].get()):
            return self.validate_bool_option(pluginOptionRow)
        else:
            self.validation_error_messages.append("There was a problem validating the options")
            return False

        return True

    def validate_int_option(self, pluginOptionRow):
        if pluginOptionRow["optionTypeVar"].get() == OPTION_TRANSTIME:
            validMin = 1
            validMax = 600
        elif pluginOptionRow["optionTypeVar"].get() == OPTION_DELAYTIME:
            validMin = 0
            validMax = 600
        elif pluginOptionRow["optionTypeVar"].get() == OPTION_WINDOWSIZE:
            validMin = 1
            validMax = 50
        else:
            err = pluginOptionRow["optionTypeVar"].get() + " is not a valid plugin option"
            self.validation_error_messages.append(err)
            return False

        if "optionMinVar" not in pluginOptionRow or "optionMaxVar" not in pluginOptionRow or "optionDefaultVar" not in pluginOptionRow:
            err = "Internal error for " + pluginOptionRow["optionTypeVar"].get()
            self.validation_error_messages.append(err)
            return False
        try:
            minVal = int(pluginOptionRow["optionMinVar"].get())
            maxVal = int(pluginOptionRow["optionMaxVar"].get())
            defaultVal = int(pluginOptionRow["optionDefaultVar"].get())
        except ValueError:
            err = "Ensure values are valid integers for option " + pluginOptionRow["optionTypeVar"].get()
            self.validation_error_messages.append(err)
            return False

        if minVal < validMin or minVal > validMax or minVal >= maxVal:
            err = "Ensure min, max, and default values are within bounds for option " + pluginOptionRow["optionTypeVar"].get()
            self.validation_error_messages.append(err)
            return False
        if maxVal > validMax or maxVal < validMin or maxVal <= minVal:
            err = "Ensure min, max, and default values are within bounds for option " + pluginOptionRow["optionTypeVar"].get()
            self.validation_error_messages.append(err)
            return False
        if defaultVal < minVal or defaultVal > maxVal:
            err = "Ensure min, max, and default values are within bounds for option " + pluginOptionRow["optionTypeVar"].get()
            self.validation_error_messages.append(err)
            return False

        return True

    def validate_string_option(self, pluginOptionRow):
        if pluginOptionRow["optionTypeVar"].get() == OPTION_LINDIR:
            if "enabledOptionsVars" not in pluginOptionRow:
                err = "Internal error for " + pluginOptionRow["optionTypeVar"].get()
                self.validation_error_messages.append(err)
                return False

            defaultIsValid = False
            numEnabled = 0
            for enabledOptionVar in pluginOptionRow["enabledOptionsVars"]:
                if enabledOptionVar.get() == 1:
                    numEnabled += 1
                    index = pluginOptionRow["enabledOptionsVars"].index(enabledOptionVar)
                    if self.linDirectionOptions[index] == pluginOptionRow["optionDefaultVar"].get():
                        defaultIsValid = True
            if not defaultIsValid:
                err = "Default value must be enabled for option " + pluginOptionRow["optionTypeVar"].get()
                self.validation_error_messages.append(err)
                return False
            if numEnabled < 2 or numEnabled > 4:
                err = "There must be at least 2 enabled options for " + pluginOptionRow["optionTypeVar"].get()
                self.validation_error_messages.append(err)
                return False

        elif pluginOptionRow["optionTypeVar"].get() == OPTION_RADDIR:
            pass
        elif pluginOptionRow["optionTypeVar"].get() == OPTION_ROTDIR:
            pass
        else:
            err = pluginOptionRow["optionTypeVar"].get() + " is not a valid plugin option"
            self.validation_error_messages.append(err)
            return False

        if "optionDefaultVar" not in pluginOptionRow:
            err = "Internal error for " + pluginOptionRow["optionTypeVar"].get()
            self.validation_error_messages.append(err)
            return False

        return True

    def validate_bool_option(self, pluginOptionRow):
        if pluginOptionRow["optionTypeVar"].get() == OPTION_LOOP:
            pass
        else:
            err = pluginOptionRow["optionTypeVar"].get() + " is not a valid plugin option"
            self.validation_error_messages.append(err)
            return False

        if "optionDefaultVar" not in pluginOptionRow:
            err = "Internal error for " + pluginOptionRow["optionTypeVar"].get()
            self.validation_error_messages.append(err)
            return False

        return True

    def plugin_option_is_type_int(self, pluginOptionType):
        return pluginOptionType == OPTION_TRANSTIME or pluginOptionType == OPTION_DELAYTIME or pluginOptionType == OPTION_WINDOWSIZE

    def plugin_option_is_type_string(self, pluginOptionType):
        return pluginOptionType == OPTION_LINDIR or pluginOptionType == OPTION_RADDIR or pluginOptionType == OPTION_ROTDIR

    def plugin_option_is_type_bool(self, pluginOptionType):
        return pluginOptionType == OPTION_LOOP
