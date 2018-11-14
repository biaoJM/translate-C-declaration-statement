/**************************************************************************************************
 * 状态机规则
 * 解析之后的符号去掉不再考虑，当完全删除所有符号之后解析成功
 * 1.从最左侧的标识符开始，"identifier是"
 * 2.如果右侧是"[]"就获取，"一个...的数组"
 * 3.如果右侧是"()"就获取，"参数为...返回值为...的函数"
 * 4.如果左侧是"("就获取整个括号中的内容，这个括号包含的是已经处理过的声明，回到步骤2
 * 5.如果左侧是"const""volatile""*"就获取，持续读取左侧的符号直到不再是这三个之中的，之后返回步骤4
 *      "const"："只读的"
 *      "volatile"："volatile"
 *      "*"："指向..."
 * 6.余下的就是基本数据类型
 ****************************************************************************************************/

#include <stdio.h>
#include <string.h>

#define MAX_LENGTH 100
#define MAX_STRING 1000

/* 返回值 */
#define FALSE 0
#define TRUE 1 
/* 模式 */
#define LEFT 0
#define RIGHT 1

static char keys[][MAX_LENGTH] =
{
    "int",
    "char",
    "double",
    "float",
    "void",
    "long",
    "short",
    "unsigned",
    "const",
    "volatile"
};
/* 需要翻译的声明语句 */
static char str[] = "char* const*(*next)()";
/* 声明语句的当前index */
static int index = 0;
/* 放置翻译结果 */
static char result[MAX_STRING]="";

/**************************************************************** 
 * 状态机
 ****************************************************************/
static void state_1(void);
static void state_2(void);
static void state_3(void);
static void state_4(void);
static void state_5(void);
static void state_6(void);

/**************************************************************** 
 * 辅助函数
 ****************************************************************/
/*
 * description： 判断一个字符是否是构成标识符的字符
 * parameter：   需判断字符
 * return：      是/否
 */
static int is_word_char(char c);
/*
 * description： 判断一个字符是否可以作为标识符起始字符的
 * parameter：   需判断字符
 * return：      是/否
 */
static int is_start_word_char(char c);
/*
 * description： 从当前index开始获取一个单词
 *               index会相应的移动到该单词后面的一个位置
 * parameter：   放置结果，如果没有就会为空
 * return：      void
 */
static void get_word(char *result);
/*
 * description： 判断一个单词是否是关键字（仅和声明有关的）
 * parameter：   需判断的单词（默认已有字符串结尾）
 * return：      是/否
 */
static int is_key(char *token);
/*
 * description： 删除字符串中的'-'
 *               因为会将解析过的部分替换为'-'方便删除
 * parameter：   void
 * return：      void
 */
static void delete_hyphen(void);
/*
 * description： 向左或向右去掉index和下一个有意义符号之间的空格
 * parameter：   模式-向左或向右
 * return：      void
 */
void eat_space(int mode);
/**************************************************************** 
 * 辅助函数实现
 ****************************************************************/
static int is_word_char(char c)
{
    return( (c>='a' && c<='z')
            || (c>='A' && c<='Z')
            || c=='_'
            || (c>='0' && c<='9')
            );
}

static int is_start_word_char(char c)
{
    return( (c>='a' && c<='z')
            || (c>='A' && c<='Z')
            || c=='_'
            );
}

static void get_word(char *result)
{
    int start=index;
    while(is_word_char(str[index]))
    {
        result[index-start] = str[index];
        index++;
    }
    result[index-start]='\0';
}

static int is_key(char *token)
{
    for(int i=0;i<sizeof(keys)/sizeof(keys[0]);i++)
    {
        if(!strcmp(token,keys[i]))
            return TRUE;
    }
    return FALSE;
}

static void delete_hyphen()
{
    /* 利用记录有效字符的的方法时间复杂度为O(n) */
    /* 记录index有多少个'-'被删除，方便恢复index的实际指向位置 */
    int count = 0;
    int i=0,j=0;
    /* 按序记录那些位置有非'-'字符 */
    int mark[MAX_STRING]={0};
    while(str[i])
    {
        if(str[i]!='-')
        {
            mark[j]=i;
            j++;
        }
        else if(i<index)
        {
            count++;
        }
        i++;
    }
    mark[j]=-1;
    
    /* 将有效字符依次向前移动 */
    i=0;
    j=0;
    while(mark[j]!=-1)
    {
        str[i]=str[mark[j]];
        i++;
        j++;
    }
    str[i]='\0';
    /* 恢复index实际指向的字符 */
    index -= count;
}

void eat_space(int mode)
{
    while(str[index]==' ')
        index += (mode==LEFT?-1:1);
}

/**************************************************************** 
 * 状态机函数实现
 ****************************************************************/
static void state_1()
{
    //printf("state 1...\n");

    char word[MAX_LENGTH]="";
    while(str[index])
    {
        // 尝试获取单词
        if(is_start_word_char(str[index]))
        {
            int start = index;
            // 获取单词
            get_word(word);
            // 如果不是关键字，而是标识符，就从这里开始
            if(!is_key(word))
            {
                strcat(result,strcat(word," is"));
                // 标识符已解析，删除
                for(int i=start;i<index;i++)
                    str[i]='-';
                delete_hyphen();
                // 进入状态2
                state_2();
                return;
            }
        }
        else
            index++;
    }
}

static void state_2()
{
    //printf("state 2...\n");
    
    eat_space(RIGHT);
    if(str[index]=='[')
    {
        strcat(result," array of");
        // 已解析数组，删除
        while(str[index]!=']')
            str[index++]='-';
        str[index++]='-';// ']' -> ' ' 并且将index移到右侧
        delete_hyphen();
    }
    // 不管是否有数组，都进入状态3
    state_3();
}

static void state_3()
{
    //printf("state 3...\n");
    
    eat_space(RIGHT);
    if(str[index]=='(')
    {
        strcat(result," function returning");
        // 已解析函数，删除
        while(str[index]!=')')
            str[index++]='-';
        str[index++]='-';// ')' -> ' ' 并且将index移到右侧
        delete_hyphen();
    }
    // 不管是否有函数，都进入状态4
    state_4();
}

static void state_4()
{
    //printf("state 4...\n");
    
    // 因为要向左尝试解析'('，失败的话还要恢复index的位置
    int old_index = index;
    index--;
    eat_space(LEFT);
    if(str[index]=='(')
    {
        // 删除已解析的部分
        while(str[index]!=')')
            str[index++]='-';
        str[index++]='-';// ')' -> ' ' 并且将index移到右侧
        delete_hyphen();
        // 是括号，进入状态2
        state_2();
    }
    else
    {
        // 否则恢复index，并按序执行状态机
        index = old_index;
        state_5();
    }
}

static void state_5()
{
    //printf("state 5...\n");
    
    // 是否仍然存在这三个符号，以此跳出循环
    int flag=1;
    // 是否解析过符号，以此判断下一个要执行的状态
    int flag2=1;
    while(flag)
    {
        flag = 0;// 默认没有
        index--;// 因为要向左解析
        eat_space(LEFT);
        if(str[index]=='*')
        {
            flag = 1;
            flag2 = 0;
            strcat(result," pointer to");
            // 删除
            str[index]='-';
            delete_hyphen();
        }
        else if(is_word_char(str[index]))
        {
            char word[MAX_LENGTH]="";
            // 将index移动到单词之前
            while(is_word_char(str[--index]));
            index++;
            // 记录单词的位置，方便删除
            int begin = index;
            get_word(word);
            if(!strcmp(word,"volatile") || !strcmp(word,"const"))
            {
                flag = 1;
                flag2 = 0;
                // 按不同的修饰符翻译不同的结果
                strcat(result,word[0]=='c'?" read only":"volatile");
                // 删除
                for(begin;begin<index;begin++)
                    str[begin]='-';
                delete_hyphen();
            }
        }
    }
    index++;
    if(flag2)
        state_6();
    else
        state_4();
}

static void state_6()
{
    //printf("state 6...\n");
    strcat(strcat(result," "),str);
}

void main(void)
{
    state_1();
    puts(result);
}