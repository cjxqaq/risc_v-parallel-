#include <iostream>
#include <stdlib.h>
#include <fstream>
using namespace std;
const int memspace = 0x3fffff;
int pc;
int r[32];
unsigned char memory[memspace];

int offset, opcode, rs2_index, rs1_index;
int rs1, rs2, res, rd_index, inst, imm, func3, func7;//当前流程的所需的值

int rlock[32], pclock;
bool memlock[memspace];//锁
int ifdone, iddone, exdone, memdone;//标志状态
int exop, memop, wbop, exf3, exf7, memf3, rd, eximm, memres, wbres, ifimm;//在流程中传递的值
int rsused = 1, rdused = 1;//防止未使用就被覆盖

int success, fail;
enum jump {  beq, bne, blt, bge, bltu, bgeu, other, jalr,jal };
int pc_not_jump;
jump a = other;
bool idright = 1, ifright = 1;
unsigned char table[6] = { 3,3,3,3,3,3 };
bool jump_flag;

int signedextend(int x, int bits)
{
	if (x >> (bits - 1) == 1)
	{
		x |= ((0xffffffff >> bits) << bits);
	}
	return x;
}
void read()
{
	char ch;
	int a = 0;
	int b = 0;
	int s[10];
	s[0] = 1;
	for (int i = 1; i <= 8; ++i)
	{
		s[i] = s[i - 1] * 16;
	}
	while (cin.get(ch))
	{
		if (ch == '@')
		{
			int tmp = 0;
			for (int i = 7; i >= 0; --i)
			{
				cin.get(ch);
				if (ch >= '0'&&ch <= '9')
					tmp += (ch - '0') * s[i];
				else
					tmp += (ch - 'A' + 10) * s[i];
			}
			offset = tmp;
			continue;
		}
		if (ch == ' ' || ch == '\n')
		{
			a = 0;
			b = 0;
			continue;
		}
		if (ch >= '0'&&ch <= '9')
		{
			if (b == 0)
			{
				a += (ch - '0') * 16;
				++b;
			}
			else
			{
				a += (ch - '0');
				memory[offset++] = (unsigned char)a;
			}
			continue;
		}
		if (ch >= 'A'&&ch <= 'F')
		{
			if (b == 0)
			{
				a += (ch - 'A' + 10) * 16;
				++b;
			}
			else
			{
				a += (ch - 'A' + 10);
				memory[offset++] = (unsigned char)a;
			}
			continue;
		}
	}
	//fclose(file);
}
void IF()
{

	if (ifdone == 1)
		return;
	if (ifright == 0)
	{
		pc = pc_not_jump;
		ifright = 1;
		pclock = 0;
		success--;
		fail++;
	}
	if (pclock == 1)
	{
		if (a == jal)
		{
			pc_not_jump = pc;
			pc = pc - 4 + ifimm;
			pclock--;
		}
		else if (a >= 0 && a <=5)
		{
			if ((table[a] & 3) == 2 || (table[a] & 3) == 3)
			{
				if (a == 5)
					pc += 0;
				pc_not_jump = pc;
				pc = pc - 4 + ifimm;//jump
				pclock--;
				jump_flag = 1;
			}
			else if ((table[a+1] & 3) == 0 || (table[a] & 3) == 1)
			{
				pc_not_jump = pc - 4 + ifimm;//为保持形式统一
				pclock--;
				jump_flag = 0;
			}
			else
			{
				pc_not_jump = pc;
				pc = pc - 4 + ifimm;//jump
				pclock--;
				jump_flag = 1;
			}
		}
		else return;//jalr
	}

	inst = 0;
	for (int i = 3; i >= 0; --i)
	{
		if (memlock[pc + i] != 0)
			return;
		inst += (int)memory[pc + i] << (8 * i);
	}
	ifdone++;
	opcode = inst & 127;
	if (opcode == 99 || opcode == 103 || opcode == 111)//99_b-type 103_jalr 111_jal
	{
		pclock++;


		if (opcode == 99)
		{
			switch ((inst >> 12) & 7)
			{
			case 0:
				a = beq;
				break;
			case 1:
				a = bne;
				break;
			case 4:
				a = blt;
				break;
			case 5:
				a = bge;
				break;
			case 6:
				a = bltu;
				break;
			case 7:
				a = bgeu;
				break;
			default:
				break;
			}
			success++;
		}
		if (opcode == 111)
			a = jal;
		if (opcode == 103)
			a = jalr;
	}
	pc += 4;
	return;
}
void ID()
{
	if (ifdone == 0 || iddone == 1)
		return;
	if (idright == 0)
	{
		ifdone--;
		idright = 1;
		ifright = 0;
		return;
	}
	opcode = inst & 127;
	rlock[0] = 0;

	switch (opcode)//读取操作码
	{
	case 51://r-type
	{
		func3 = (inst >> 12) & 7;
		rs2_index = (inst >> 20) & 31;
		rs1_index = (inst >> 15) & 31;
		if (rlock[rs2_index] != 0 || rlock[rs1_index] != 0 || rsused == 0 || rdused == 0)//check if locked
		{
			return;
		}

		ifdone--;
		iddone++;

		rs1 = r[rs1_index];
		rs2 = r[rs2_index];
		rsused = 0;
		rd_index = (inst >> 7) & 31;
		rlock[rd_index]++;
		rd = rd_index;
		rdused = 0;

		func7 = (inst >> 25) & 127;
		exf3 = func3;
		exf7 = func7;
		exop = opcode;
		return;
	}
	case 3:case 103://i-type
	{
		imm = (inst >> 20) & 0xfff;
		rs1_index = (inst >> 15) & 31;
		if (rlock[rs1_index] != 0 || rsused == 0 || rdused == 0)//check if locked
		{
			return;
		}

		ifdone--;
		iddone++;

		rs1 = r[rs1_index];
		rsused = 0;
		rd_index = (inst >> 7) & 31;
		rlock[rd_index]++;
		rd = rd_index;
		rdused = 0;

		func3 = (inst >> 12) & 7;
		exf3 = func3;

		imm = signedextend(imm, 12);
		eximm = imm;

		exop = opcode;

		return;
	}
	case 35://s-type
	{
		imm = inst >> 25;
		imm = (imm << 5) | ((inst >> 7) & 31);
		rs2_index = (inst >> 20) & 31;
		rs1_index = (inst >> 15) & 31;
		if (rlock[rs2_index] != 0 || rlock[rs1_index] != 0 || rsused == 0)//check if locked
		{
			return;
		}

		ifdone--;
		iddone++;

		rs1 = r[rs1_index];
		rs2 = r[rs2_index];
		rsused = 0;
		func3 = (inst >> 12) & 7;
		exf3 = func3;

		imm = signedextend(imm, 12);
		eximm = imm;

		exop = opcode;
		return;
	}
	case 99://b-type
	{
		imm = inst >> 31;
		imm = (imm << 1) | ((inst >> 7) & 1);
		imm = (imm << 6) | ((inst >> 25) & 63);
		imm = (imm << 4) | ((inst >> 8) & 15);
		imm <<= 1;
		imm = signedextend(imm, 13);
		rs2_index = (inst >> 20) & 31;
		rs1_index = (inst >> 15) & 31;
		if (rlock[rs2_index] != 0 || rlock[rs1_index] != 0)//check if locked
		{
			return;
		}

		ifdone--;
		iddone++;

		rs1 = r[rs1_index];
		rs2 = r[rs2_index];
		rsused = 0;
		func3 = (inst >> 12) & 7;
		exf3 = func3;

		exop = opcode;

		ifimm = imm;

		return;
	}
	case 55:case 23://u-type
	{
		if (rdused == 0)
			return;
		imm = inst >> 12;
		imm <<= 12;

		ifdone--;
		iddone++;

		rd_index = (inst >> 7) & 31;
		rd = rd_index;
		rlock[rd_index]++;
		rdused = 0;

		exop = opcode;
		eximm = imm;

		return;
	}
	case 111://j-type
	{
		if (rdused == 0)
			return;
		imm = ((inst >> 31) & 1);
		imm = (imm << 8) + ((inst >> 12) & 255);
		imm = (imm << 1) + ((inst >> 20) & 1);
		imm = (imm << 10) + ((inst >> 21) & 1023);
		imm <<= 1;
		ifdone--;
		iddone++;
		rd_index = (inst >> 7) & 31;
		rlock[rd_index]++;
		rd = rd_index;
		rdused = 0;

		imm = signedextend(imm, 21);
		ifimm = imm;

		exop = opcode;
		return;
	}
	case 19://with constant
	{
		func3 = (inst >> 12) & 7;
		if (func3 == 1 || func3 == 5)//r-type(not)
		{
			if (rlock[rs1_index] != 0 || rsused == 0 || rdused == 0)//check if locked
			{
				return;
			}
			rs2 = (inst >> 20) & 31;//shamt
			rs1_index = (inst >> 15) & 31;


			ifdone--;
			iddone++;

			rs1 = r[rs1_index];
			//rsused = 0;
			rd_index = (inst >> 7) & 31;
			rlock[rd_index]++;
			rd = rd_index;
			rdused = 0;
			func7 = (inst >> 25) & 127;
			exf3 = func3;

			exf7 = func7;
			exop = opcode;
			return;
		}
		else//i-type
		{
			imm = inst >> 20;
			rs1_index = (inst >> 15) & 31;
			if (rlock[rs1_index] != 0 || rsused == 0 || rdused == 0)//check if locked
			{
				return;
			}

			ifdone--;
			iddone++;

			rs1 = r[rs1_index];
			rd_index = (inst >> 7) & 31;
			rlock[rd_index]++;
			rd = rd_index;
			rdused = 0;
			func3 = (inst >> 12) & 7;
			exf3 = func3;

			imm = signedextend(imm, 12);
			eximm = imm;

			exop = opcode;
			return;
		}
	}
	default:
		cout << "ID error:" << (inst & 0x7f) << endl;
		return;
	}
}
void EX()
{
	if (iddone == 0 || exdone == 1)
		return;
	iddone--;
	exdone++;
	memop = exop;
	switch (exop)
	{
	case 51://r-type
	{
		rsused = 1;
		switch (exf3)
		{
		case 0://add and sub
		{

			if (exf7 == 0)//add
			{
				res = rs1 + rs2;
				wbres = res;

				return;
			}
			if (exf7 == 32)//sub
			{
				res = rs1 - rs2;
				wbres = res;

				return;
			}
			cout << "error: EX case 51";
			return;
		}
		case 1://sll
		{
			res = (unsigned)rs1 << (unsigned)(rs2 & 31);
			wbres = res;

			return;
		}
		case 2://slt
		{
			res = rs1 < rs2 ? 1 : 0;
			wbres = res;

			return;
		}
		case 3://sltu
		{
			res = (unsigned)rs1 < (unsigned)rs2 ? 1 : 0;
			wbres = res;

			return;
		}
		case 4://xor
		{
			res = rs1 xor rs2;
			wbres = res;

			return;
		}
		case 5://srl and sra
		{
			if (exf7 == 0)//srl
			{
				res = (unsigned)rs1 >> (unsigned)(rs2 & 31);
				wbres = res;

				return;
			}
			if (exf7 == 32)//sra
			{
				res = rs1 >> (rs2 & 31);
				wbres = res;

				return;
			}
			cout << "error: EX case 51";
			return;
		}
		case 6://or
		{
			res = rs1 | rs2;
			wbres = res;

			return;
		}
		case 7://and
		{
			res = rs1 & rs2;
			wbres = res;

			return;
		}
		default:
			cout << "error: EX case 51";
			return;
		}
		return;
	}
	case 103://(i-type)jalr
	{
		imm = eximm;
		res = pc;
		pc = rs1 + imm;
		pc >>= 1;
		pc <<= 1;
		pclock--;
		return;
	}
	case 3://(i-type)lb lh lw lbu lhu
	{
		imm = eximm;
		res = rs1 + imm;
		memres = res;

		memf3 = exf3;
		return;
	}
	case 35://(s-type)sb sh sw
	{
		imm = eximm;
		memf3 = exf3;
		res = rs1 + imm;
		memres = res;
		if (exf3 == 0)
		{
			memlock[res] = 1;
		}
		else if (exf3 == 1)
		{
			memlock[res] = memlock[res + 1] = 1;
		}
		else if (exf3 == 2)
		{
			memlock[res] = memlock[res + 1] = memlock[res + 2] = memlock[res + 3] = 1;
		}
		return;
	}
	case 99://b-type
	{
		rsused = 1;
		switch (exf3)
		{
		case 0://beq
		{
			if (rs1 == rs2)
			{
				table[beq] <<= 1;
				table[beq] |= 1;
				if (jump_flag == 0)
				{
					idright = 0;
				}
				return;
			}
			table[beq] <<= 1;
			if (jump_flag == 1)
			{
				idright = 0;

			}
			return;
		}
		case 1://bne
		{
			if (rs1 != rs2)
			{
				table[bne] <<= 1;
				table[bne] |= 1;
				if (jump_flag == 0)
				{
					idright = 0;
				}
				return;
			}
			table[bne] <<= 1;
			if (jump_flag == 1)
			{
				idright = 0;

			}
			return;
		}
		case 4://blt
		{
			if (rs1 < rs2)
			{
				table[blt] <<= 1;
				table[blt] |= 1;
				if (jump_flag == 0)
				{
					idright = 0;
				}
				return;
			}
			table[blt] <<= 1;
			if (jump_flag == 1)
			{
				idright = 0;

			}
			return;

		}
		case 5://bge
		{
			if (rs1 >= rs2)
			{
				table[bge] <<= 1;
				table[bge] |= 1;
				if (jump_flag == 0)
				{
					idright = 0;
				}
				return;
			}
			table[bge] <<= 1;
			if (jump_flag == 1)
			{
				idright = 0;

			}
			return;
		}
		case 6://bltu
		{
			if ((unsigned)rs1 < (unsigned)rs2)
			{
				table[bltu] <<= 1;
				table[bltu] |= 1;
				if (jump_flag == 0)
				{
					idright = 0;
				}
				return;
			}
			table[bltu] <<= 1;
			if (jump_flag == 1)
			{
				idright = 0;

			}
			return;
		}
		case 7://bgeu
		{
			if ((unsigned)rs1 >= (unsigned)rs2)
			{
				table[bgeu] <<= 1;
				table[bgeu] |= 1;
				if (jump_flag == 0)
				{
					idright = 0;
				}
				return;
			}
			table[bgeu] <<= 1;
			if (jump_flag == 1)
			{
				idright = 0;

			}
			return;
		}
		default:
			cout << "error: EX case 99";
			return;
		}
	}
	case 55://(u-type)lui
	{
		imm = eximm;
		res = imm;
		wbres = res;

		return;
	}
	case 23://(u-type)auipc
	{
		imm = eximm;
		res = pc - 4 + imm;
		wbres = res;

		return;
	}
	case 111://(j-type)jal
	{
		res = pc_not_jump;
		wbres = res;
		return;
	}
	case 19://with constant
	{

		rsused = 0;
		switch (exf3)
		{
			rsused = 0;
		case 0://addi
		{
			imm = eximm;
			res = rs1 + imm;
			wbres = res;
			return;
		}
		case 2://slti
		{
			imm = eximm;
			res = rs1 < imm ? 1 : 0;
			wbres = res;

			return;
		}
		case 3://sltui
		{
			imm = eximm;
			res = (unsigned)rs1 < (unsigned)imm ? 1 : 0;
			wbres = res;

			return;
		}
		case 4://xori
		{
			imm = eximm;
			res = rs1 xor imm;
			wbres = res;

			return;
		}
		case 6://ori
		{
			imm = eximm;
			res = rs1 | imm;
			wbres = res;

			return;
		}
		case 7://andi
		{
			imm = eximm;
			res = rs1 & imm;
			wbres = res;

			return;
		}
		case 1://slli
		{
			res = rs1 << rs2;//(not actuall rs2)
			wbres = res;

			return;
		}
		case 5:
		{
			if (exf7 == 0)//srli
			{
				res = (unsigned)rs1 >> (unsigned)rs2;//(not actuall rs2)
				wbres = res;

				return;
			}
			if (exf7 == 32)//srai
			{
				res = rs1 >> rs2;//(not actuall rs2)
				wbres = res;

				return;
			}
			cout << "error: EX case 19";
			return;
		}
		default:
			cout << "error: EX case 19";
			return;
		}
	}
	}
}
//load and store
void MEM()
{
	if (exdone == 0 || memdone == 1)
		return;
	exdone--;
	memdone++;
	wbop = memop;
	rsused = 1;
	switch (memop)
	{
	case 3://(i-type)load
	{
		res = memres;
		switch (memf3)
		{
		case 0://lb
		{
			res = memory[res];
			if (res & 0x80) res |= 0xffffff00;
			wbres = res;
			return;
		}
		case 1://lh
		{
			res = (memory[res] & 255) | ((memory[res + 1] & 255) << 8);
			res = signedextend(res, 16);
			wbres = res;
			return;
		}
		case 2://lw
		{
			res = (memory[res] & 255) | ((memory[res + 1] & 255) << 8) | ((memory[res + 2] & 255) << 16) | ((memory[res + 3] & 255) << 24);
			wbres = res;
			return;
		}
		case 4://lbu
		{
			res = memory[res] & 255;
			wbres = res;
			return;
		}
		case 5://lhu
		{
			res = memory[res] | ((memory[res + 1] & 255) << 8);
			wbres = res;
			return;
		}
		default:
			cout << "error: MEM load";
			return;
		}
	}
	case 35://(s-type)store
	{
		res = memres;

		if (memf3 == 0)//sb
		{
			memory[res] = rs2 & 255;
			if (res == 0x30004)
			{
				cout << (unsigned)(r[10] & 255) << '\n';
				//cout << (double)success / (success + fail);
				exit(0);
			}
			memlock[res] = 0;
			return;
		}
		if (memf3 == 1)//sh;
		{
			memory[res] = rs2 & 255;
			memory[res + 1] = (rs2 >> 8) & 255;
			memlock[res] = memlock[res + 1] = 0;
			return;
		}
		if (memf3 == 2)//sw
		{
			memory[res] = rs2 & 255;
			memory[res + 1] = (rs2 >> 8) & 255;
			memory[res + 2] = (rs2 >> 16) & 255;
			memory[res + 3] = (rs2 >> 24) & 255;
			memlock[res] = memlock[res + 1] = memlock[res + 2] = memlock[res + 3] = 0;
			return;
		}
		cout << "error: MEM store";
		return;
	}
	}
	return;
}
void WB()
{
	if (memdone == 0)
		return;
	memdone--;

	switch (wbop)//读取操作码
	{
	case 99:case 35://store and b-type
		return;
	default:
		rd_index = rd;
		rdused = 1;
		res = wbres;

		if (rd_index == 0)
			return;
		r[rd_index] = res;
		rlock[rd_index]--;
		return;
	}
}

int main()
{
	read();
	/*while (1)
	{
		IF();
		ID();
		EX();
		MEM();
		WB();
	}*/
	while (1)
	{
		WB();
		MEM();
		EX();
		ID();
		IF();
	}
}

