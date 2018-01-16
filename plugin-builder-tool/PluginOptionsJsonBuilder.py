import json
import sys

def load_from_file():
	try:
		with open("plugin_options.sav") as fh:
			str = fh.readline()
			return json.loads(str)
	except:
		return []


def store_to_file(options):
	with open("plugin_options.sav", "w") as fh:
		fh.write(json.dumps(options))


def get_index_of_option_in_options(options, option_name):
	i = 0
	for option in options:
		if option['name'] == option_name:
			return i
		i = i + 1
	return -1


def make_generic_int_option(name, default_value, max_value, min_value):
	option = {'name':name, 'type':'int', 'defaultValue':default_value, 'maxValue':max_value, 'minValue':min_value}
	return option


def make_generic_double_option(name, default_value, max_value, min_value):
	option = {'name':name, 'type':'double', 'defaultValue':default_value, 'maxValue':max_value, 'minValue':min_value}
	return option

def make_generic_string_option(name, default_value, strings):
	option = {'name':name, 'type':'string', 'defaultValue':default_value, 'strings':strings}
	return option


def make_generic_bool_option(name, default_value):
	option = {'name':name, 'type':'bool', 'defaultValue':default_value}
	return option


def make_transtime(default_value, max_value=600, min_value=1):
	option = make_generic_int_option('transTime', default_value, max_value, min_value)
	return option


def make_loop(default_value):
	option = make_generic_bool_option('loop', default_value)
	return option


def make_lindirection(default_value, enabled_strings=['left', 'right', 'up', 'down']):
	option = make_generic_string_option('linDirection', default_value, enabled_strings)
	return option


def make_rotdirection(default_value):
	strings = ['in', 'out']
	option = make_generic_string_option('rotDirection', default_value, strings)
	return option


def make_raddirection(default_value):
	strings = ['cw', 'ccw']
	option = make_generic_string_option('radDirection', default_value, strings)
	return option


def make_delaytime(default_value, max_value=600, min_value=0):
	option = make_generic_int_option('delayTime', default_value, max_value, min_value)
	return option


def make_ncolorsperframe(default_value, max_value=50, min_value=0):
	option = make_generic_int_option('nColorsPerFrame', default_value, max_value, min_value)
	return option


def option_exists(options, name):
	index = get_index_of_option_in_options(options, name)
	if index >= 0:
		return True
	else:
		return False

def add_option(options):
	print("Please choose one from the list of the following plugin options:\n" + \
	"1. TransTime\n" + \
	"2. loop\n" + \
	"3. linDirection\n" + \
	"4. radDirection\n" + \
	"5. rotDirection\n" + \
	"6. delayTime\n" + \
	"7. nColorsPerFrame")

	while True:
		try:
			option_choice = input(">>>")
		except:
			print("please input a number corresponding to a plugin option")
			continue
		if option_choice > 7 or option_choice < 1:
			continue
		break

	chosen_option = {}

	if option_choice == 1:
		print("please enter a default value for the 'transTime' option(int, 1-600): ")
		default_value = input(">>>")
		chosen_option = make_transtime(default_value)

	elif option_choice == 2:
		print("please enter a default value for the 'loop' option(boolean, please enter 1 for true, and 0. for false): ")
		default_value = input(">>>")
		chosen_option = make_loop(default_value)

	elif option_choice == 3:
		print("please enter a default value for the 'linDirection' option (choose one of the following: left, right, up, down): ")
		default_value = raw_input(">>>")
		chosen_option = make_lindirection(default_value)

	elif option_choice == 4:
		print("please enter a default value for the 'radDirection' option (choose one of the following: in, out): ")
		default_value = raw_input(">>>")
		chosen_option = make_raddirection(default_value)

	elif option_choice == 5:
		print("please enter a default value for the 'rotDirection' option (choose one of the following: cw, ccw): ")
		default_value = raw_input(">>>")
		chosen_option = make_rotdirection(default_value)

	elif option_choice == 6:
		print("please enter a default value for the 'delayTime' option (int, 0-600): ")
		default_value = input(">>>")
		chosen_option = make_delaytime(default_value)

	elif option_choice == 7:
		print("please enter a default value for the 'nColorsPerFrame' option (int): ")
		default_value = input(">>>")
		chosen_option = make_ncolorsperframe(default_value)


	if not option_exists(options, chosen_option['name']):
		options.append(chosen_option)


def remove_option(options):
	print("please input name of option you wish to remove")
	name = raw_input(">>>")
	index = get_index_of_option_in_options(options, name)
	if index < 0:
		print("could not find option : " + name)
		return

	del options[index]


def load_from_header_file(file_path):
	try:
		with open(file_path) as fh:
			str = None
			for line in fh:
				if "const char* pluginOptionsJsonString" in line:
					str = line
					break
			if not str:
				return []
			str = str.replace(" ", "")
			str = str.replace("constchar*pluginOptionsJsonString=\"", "")
			str = str.replace("\\\"", "\"")
			str = str.replace("\";", "")
			str = str.replace("{\"options\":", "")
			str = str.replace("}]}", "}]")
			return json.loads(str)
	except:
		return []

def write_to_header_file(json_str_escaped_quotes, file_path):
	with open(file_path, "w") as fh:
		fh.write("#ifndef PLUGIN_OPTIONS_H\n#define PLUGIN_OPTIONS_H\n\n")
		fh.write("const char* pluginOptionsJsonString = \"" + json_str_escaped_quotes + "\";\n\n")
		fh.write("#ifdef __cplusplus\nextern \"C\" {\n#endif\n\n")
		fh.write("const char* getPluginOptionsJsonString(){\n\treturn pluginOptionsJsonString;\n}\n\n")
		fh.write("#ifdef __cplusplus\n}\n#endif\n\n#endif\n")



def write_options_to_header_file(options, file_path):
	optionsJson = {'options':options}
	json_str = json.dumps(optionsJson)
	json_str_escaped_quotes = ""
	for c in json_str:
		if c == "\"":
			json_str_escaped_quotes += "\\"
		json_str_escaped_quotes += c
	write_to_header_file(json_str_escaped_quotes, file_path)


def print_options(options):
	if len(options) == 0:
		return

	print("##################################")
	for option in options:
		print(option['name'])
		print("\tType: " + option['type'])
		print("\tDefault Value: " + str(option['defaultValue']))
		if option['type'] == "int" or option['type'] == "double":
			print("\tMax Value: " + str(option['maxValue']))
			print("\tMin Value: " + str(option['minValue']))
		if option['type'] == "string":
			print("\tStrings: " + str(option['strings']))
		print("##################################")


if __name__ == "__main__":
	header_file_path = ""
	quit = False
	options = []

	if len(sys.argv) >= 2:
		if sys.argv[1] == "--help":
			print("Usage: python sdk_json_builder.py <absolute file path to top level plugin directory>")
			exit(0)
		file_path = sys.argv[1]
		if file_path[-1] == "/":
			header_file_path = file_path + "inc/PluginOptions.h"
		else:
			header_file_path = file_path + "/inc/PluginOptions.h"

	else:
		print("Usage: python sdk_json_builder.py <absolute file path to top level plugin directory>")
		exit(0)

	options = load_from_file()

	print("************************************* PLUGIN OPTIONS JSON BUILDER *************************************\n")

	while (not quit):
		print("Choose one of the following:\n" + \
		"1. Add option\n" + \
		"2. Remove option\n" + \
		"3. List Options\n" + \
		"4. Write options to header file\n" + \
		"5. Quit")
		try:
			choice = input(">>>")
		except:
			print("please input a number\n")
			continue

		if choice == 1:
			add_option(options)
			store_to_file(options)

		elif choice == 2:
			remove_option(options)
			store_to_file(options)

		elif choice == 3:
			print_options(options)

		elif choice == 4:
			write_options_to_header_file(options, header_file_path)

		elif choice == 5:
			quit = True

		else:
			print("please input a valid number corresponding to a menu\n")
			continue

