/*
 * 语法分析器
 * 构造表达式部分的语法分析器
 * 可在文件结尾附加 "#"
 * 注：尚未实现条件语句的语法分析，因未解决二义性文法的问题
 * 作者：张泰然
 * 时间：2014-11-23
 * No copyright, you can modified it freely
 * And it's better to tell me your changes
 * 注释有不规范之处，还请谅解
 */
#include <iostream>
#include <fstream>
#include <string>
#include <cctype>
#include <map>
#include <vector>
using namespace std;

#define	BEGIN	1		// 关键字 begin
#define	END		2		// 关键字 end
#define	IF		3		// 关键字 if
#define	THEN	4		// 关键字 then
#define	ELSE	5		// 关键字 else
#define	ID		6		// 标识符
#define	INT		7		// 整型常数
#define	LT		8		// 关系运算符 <
#define	LE		9		// 关系运算符 <=
#define	EQ		10		// 关系运算符 =
#define	NE		11		// 关系运算符 <>
#define	GT		12		// 关系运算符 >
#define	GE		13		// 关系运算符 >=
#define	IS		14		// 赋值符 :=
#define	PL		15		// 算术运算符 +
#define	MI		16		// 算术运算符 -
#define	MU		17		// 算术运算符 *
#define	DI		18		// 算术运算符 /
#define	LPAREN	19		// 左括号 (
#define RPAREN	20		// 右括号 )
#define	STREND	21		// 输入串的结束符 #

#define WORDLEN	50		// 单词的长度

/*
 * 二元组表示：(单词种别码，单词自身的值)
 * 约定如下：
 * （1）标识符	——（6, 单词自身的值，即标识符）
 * （2）整型常数——（7， 单词自身的值，即整数）
 * （3）其他	——（种别码，留空）
 * 注：暂时只支持整数
 */

/*
 * 基于如下BNF定义进行表达式的语法分析
 * 分析对象〈算术表达式〉的BNF定义如下：
 * <表达式> ::= [+|-]<项>{<加法运算符> <项>}
 * <项> ::= <因子>{<乘法运算符> <因子>}
 * <因子> ::= <标识符>|<无符号整数>| ‘(’<表达式>‘)’
 * <加法运算符> ::= +|-
 * <乘法运算符> ::= *|/
 *
 * 算术表达式文法：
 * E→TE′|+TE′|-TE′		Expression			→ [+|-]	Term	ExpressionDetail
 * E′→+T E′|-T E′|ε	ExpressionDetail	→ [+|-]	Term	ExpressionDetail	|	None
 * T→FT′					Term				→ Factor	TermDetail
 * T′→* FT′|/ FT′|ε	TermDetail			→ [*|/]	Factor	TermDetail	|	None
 * F→i |（E）				Factor				→ ID | INT | ( Expression )
 */

void Expression();
void ExpressionDetail();
void Term();
void TermDetail();
void Factor();

int row = 1;						// 用来表示行号
char token[WORDLEN];				// 用来依次存放一个单词词文中的各个字符
map<string, int> keywords;			// 关键字表，在初始化函数中初始化
bool foundSyntaxError = false;		// 用来表示是否已发现语法错误

struct Word							// 单词结构体
{
	int type;						// 单词种别码
	char value[WORDLEN];			// 单词自身的值
	int lineNum;					// 所在行号，报错用
};

vector<Word> words;					// 单词的有序集合，词法分析器输出至这里
Word symbol;						// 当前符号，语法分析用

/*
 * 函数 void init()
 * 作用：初始化词法分析器
 * author: Zhang Tairan
 */
void init()
{
	keywords["begin"] = BEGIN;
	keywords["end"]	  = END;
	keywords["if"]	  = IF;
	keywords["then"]  = THEN;
	keywords["else"]  = ELSE;
}

/*
 * 函数 int lookup(const char * token)
 * 参数：字符数组token
 * 作用：每调用一次，就以token中的字符串查保留字表
 * 返回值：若查到，就返回相应关键字的类别码，否则返回0
 * author: Zhang Tairan
 */
int lookup(const char * token)
{
	string word(token);
	int result;
	if (keywords.count(word))
	{
		result = keywords[word];
	}
	else
	{
		result = 0;
	}
	return result;
}

/*
 * 函数 void report_error()
 * 参数：reason  出错原因的描述 
 * 作用：报告出错的行数
 * author: Zhang Tairan
 */
void report_error(string reason = "")
{
	if (reason != "")
		cerr << "Error in line " << row << " : " << reason << endl;
	else
		cerr << "Error in line " << row << endl;
}

/*
 * 函数 void outToMem(int num, const char * val)
 * 参数：num  为相应单词的类别码或其助记符
 * 参数：val  当所识别的单词为标识符和整数时，为token（即词文分别为字母数字串和数字串），对于其余种类的单词，均为空串
 * 作用：输出一个单词的内部表示至内存
 * author: Zhang Tairan
 */
void outToMem(int num, const char * val)
{
	Word newWord;
	newWord.type = num;
	memcpy(newWord.value, val, WORDLEN);
	newWord.lineNum = row;
	words.push_back(newWord);
}

/*
 * 函数 void out(int num, const char * val)
 * 参数：num  为相应单词的类别码或其助记符
 * 参数：val  当所识别的单词为标识符和整数时，为token（即词文分别为字母数字串和数字串），对于其余种类的单词，均为空串
 * 作用：输出一个单词的内部表示，在词法分析实验中输出至控制台标准输出，在语法分析实验中输出至内存，作为接口函数
 * author: Zhang Tairan
 */
void out(int num, const char * val)
{
	outToMem(num, val);
}

/*
 * 函数 void scanner(const string &line, string::size_type &index)
 * 参数：line  为待扫描的一行源代码
 * 参数：index 为当前扫描的字符的位置，即index的索引
 * 作用：进行扫描，对所读入的符号进行相应处理
 * author: Zhang Tairan
 */
void scanner(const string &line, string::size_type &index)
{
	char ch;							// 存储当前字符
	int i;
	int c;
	if (index >= line.size())
		return;							// 保护措施，若已到行尾，则退出
	ch = line[index++];
	if (isalpha(ch))					// 关键字或标识符
	{
		i = 0;							// 跟踪token数组的下标，指向待存放的位置
		token[i] = ch;
		ch = line[index++];				// 读取下一个字符存入
		i++;
		while (isalnum(ch))
		{
			token[i] = ch;
			i++;
			ch = line[index++];			// 又是下一个
		}
		token[i] = '\0';				// 给token结尾
		index--;						// 回溯，因为当读入的字符不是字母或数字时，尽管循环终止了，index还是自增了
		if (c = lookup(token))
		{
			out(c, "");
		}
		else
		{
			out(ID, token);
		}
	}
	else if (isdigit(ch))				// 整数字面值
	{
		i = 0;							// 跟踪token数组的下标，指向待存放的位置
		token[i] = ch;
		ch = line[index++];				// 读取下一个字符存入
		i++;
		while (isdigit(ch))
		{
			token[i] = ch;
			i++;
			ch = line[index++];			// 又是下一个
		}
		if (isalpha(ch))				// 数字后面紧跟字母
		{
			report_error("数字后面跟随字母非法！");
		}
		token[i] = '\0';				// 给token结尾
		index--;						// 回溯，因为当读入的字符不是数字时，尽管循环终止了，index还是自增了
		out(INT, token);
	}
	else								// 运算符和其它字符的处理
	{
		switch (ch)
		{
		case '<':
			ch = line[index++];
			if (ch == '=')				// 关系运算符 <=
			{
				out(LE, "");
			}
			else if (ch == '>')			// 关系运算符 <>
			{
				out(NE, "");
			}
			else						// 关系运算符 <
			{
				index--;				// 回溯，多读了
				out(LT, "");
			}
			break;
		case '=':						// 关系运算符 =
			out(EQ, "");
			break;
		case '>':
			ch = line[index++];
			if (ch == '=')				// 关系运算符 >=
			{
				out(GE, "");
			}
			else						// 关系运算符 >
			{
				index--;				// 回溯，多读了
				out(GT, "");
			}
			break;
		case ':':
			ch = line[index++];
			if (ch == '=')				// 赋值符 :=
			{
				out(IS, "");
			}
			else
			{
				index--;
				report_error("冒号后需跟等号组成赋值符，否则非法！");
			}
			break;
		case '+':						// 算术运算符 +
			out(PL, "+");
			break;
		case '-':						// 算术运算符 -
			out(MI, "-");
			break;
		case '*':						// 算术运算符 *
			out(MU, "*");
			break;
		case '/':						// 算术运算符 /
			out(DI, "/");
			break;
		case '(':
			out(LPAREN, "(");
			break;
		case ')':
			out(RPAREN, ")");
			break;
		case '#':
			out(STREND, "");
			break;
		default:						// 其它字符
			report_error("非法字符！");
			break;
		}								// switch
	}									// else
}


void read(Word &symbol)
{
	static vector<Word>::size_type index = 0;
	if (index < words.size())
	{
		symbol = words[index];
		index++;
	}
		
}

void syntax_error(string reason = "")
{
	foundSyntaxError = true;
	if (reason != "")
		cerr << "发现语法错误" << " : " << reason << endl;
	else
		cerr << "发现语法错误！" << endl;
}

void Expression()
{
	if (symbol.type == PL || symbol.type == MI)
	{
		read(symbol);
	}
	Term();
	ExpressionDetail();
}

void ExpressionDetail()
{
	if (symbol.type == PL || symbol.type == MI)
	{
		read(symbol);
		Term();
		ExpressionDetail();
	}
	else if (symbol.type == RPAREN || symbol.type == STREND)
	{
		return;
	}
	else
	{
		string reason = "";
		reason += "\"";
		reason += symbol.value;
		reason += "\"：";
		reason += "此处应为加号、减号或右括号";
		syntax_error(reason);
	}
}

void Term()
{
	Factor();
	TermDetail();
}

void TermDetail()
{
	if (symbol.type == MU || symbol.type == DI)
	{
		read(symbol);
		Factor();
		TermDetail();
	}
	else if (symbol.type == PL || symbol.type == MI || symbol.type == RPAREN || symbol.type == STREND)
	{
		return;
	}
	else
	{
		string reason = "";
		reason += "\"";
		reason += symbol.value;
		reason += "\"：";
		reason += "此处应为四则运算符号或右括号";
		syntax_error(reason);
	}
}

void Factor()
{
	if (symbol.type == ID || symbol.type == INT)
	{
		read(symbol);
	}
	else if (symbol.type == LPAREN)
	{
		read(symbol);
		Expression();
		if (symbol.type == RPAREN)
		{
			read(symbol);
		}
		else
		{
			string reason = "";
			if (symbol.type == STREND)
			{
				reason += "输入串结束处应输入右括号";
			}
			else
			{
				reason += "\"";
				reason += symbol.value;
				reason += "\"：";
				reason += "此处应为右括号";
			}
			syntax_error(reason);
		}
	}
	else
	{
		string reason = "";
		if (symbol.type == STREND)
		{
			reason += "输入串结束处应输入整数、标识符";
		}
		else
		{
			reason += "\"";
			reason += symbol.value;
			reason += "\"：";
			reason += "此处应为整数、标识符或左括号";
		}
		syntax_error(reason);
	}
}

/*
 * 函数 void syntaxAnalysis()
 * 作用：进行语法分析
 * author: Zhang Tairan
 */
void syntaxAnalysis()
{
	read(symbol);
	Expression();
	if (!foundSyntaxError && symbol.type == STREND)
	{
		cout << "语法正确!" << endl;
	}
	else
	{
		cout << "语法错误!" << endl;
	}
}

int main()
{
	string filename;
	cout << "请输入文件名： ";
	getline(cin, filename);
	ifstream infile(filename.c_str());
	if (!infile)
	{
		cerr << "Error: unable to open input file: " << filename << endl;
		return -1;
	}
	init();								//在此处初始化词法分析器
	string line;						// 用于存放每一行的代码
	while (getline(infile, line))		// 一次读取一行
	{
		string::size_type index = 0;	// 用于表示当前读取字符串的位置索引
		while (index != line.size())
		{
			while (isspace(line[index++]))
				;						// 跳过空白字符
			index--;					// 回溯，因为在非空白字符处判断时，即使因为是非空白字符而终止循环，index还是自增了
			scanner(line, index);		// index传引用，在scanner中更改
		}
		out(STREND, "");				// 向每行单词集合的末尾追加一个输入串的结束符，用于方便结束分析
		row++;
	}
	syntaxAnalysis();
	infile.close();
	return 0;
}