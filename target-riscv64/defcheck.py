# This file is used to check if .def file is correct
from sys import stdin

instruct_list = \
[
	"LUI", "AUIPC", "JAL", "JALR",
	"BEQ", "BNE", "BLT", "BGE", "BLTU", "BGEU",
	"LB", "LH", "LW", "LBU", "LHU", 
	"SB", "SH", "SW",
	"ADDI", "SLTI", "SLTIU",
	"XORI", "ORI", "ANDI", "SLLI", "SRLI", "SRAI",
	"ADD", "SUB", "SLL", "SLT", "SLTU", "XOR", "SRL", "SRA", "OR", "AND",
	"FENCE", "ECALL", "EBREAK",
	"FENCE_I", "CSRRW", "CSRRS", "CSRRC", "CSRRWI", "CSRRSI", "CSRRCI"
]


def check_impl(x_list, dict_):
	for x in x_list:
		if dict_.get(x + "_IMPL") != 2:
			print(x, "is error")
def get_str_times(str_list):
	s = dict()
	for line in str_list:
		for key in line.split():
			if s.get(key) == None:
				s[key] = 1
			else:
				s[key] = s[key] + 1
	return s
def check_definst(x_list, dict_):
	for x in x_list:
		if dict_.get("DEFINST("+x+',') != 1:
			print("DEFINST",x, "is error")
if __name__ == "__main__":
	dict_ = get_str_times(stdin)

	check_impl(instruct_list, dict_)
	check_definst(instruct_list, dict_)
