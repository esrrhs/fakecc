// expect: 0
package main;

extern void* memcpy(void*, const void*, unsigned long);
extern void* memset(void*, int, unsigned long);
extern int memcmp(const void*, const void*, unsigned long);
extern void* memmove(void*, const void*, unsigned long);
extern int strcmp(const char*, const char*);
extern int strncmp(const char*, const char*, unsigned long);
extern unsigned long strlen(const char*);
extern char* strcpy(char*, const char*);
extern char* strncpy(char*, const char*, unsigned long);
extern char* strchr(const char*, int);
extern char* strrchr(const char*, int);
extern char* strcat(char*, const char*);
extern char* strncat(char*, const char*, unsigned long);
extern char* strstr(const char*, const char*);
extern void* memchr(const void*, int, unsigned long);
extern int printf(const char*, ...);
extern int sprintf(char*, const char*, ...);
extern int snprintf(char*, unsigned long, const char*, ...);
extern int fprintf(void*, const char*, ...);
extern int puts(const char*);
extern int putchar(int);
extern void* malloc(unsigned long);
extern void free(void*);
extern void* calloc(unsigned long, unsigned long);
extern void* realloc(void*, unsigned long);
extern int abs(int);
extern long labs(long);
extern int atoi(const char*);
extern long atol(const char*);
extern double atof(const char*);
extern double sqrt(double);
extern double fabs(double);
extern double pow(double, double);
extern double ceil(double);
extern double floor(double);
extern void exit(int);
extern void abort(void);
extern int rand(void);
extern void srand(unsigned int);
extern int isprint(int);

typedef _Bool bool;
const bool false = 0;
const bool true = 1;

enum EPosition 
{
  STATIC, RELATIVE, ABSOLUTE, FIXED
};
typedef enum EPosition EPosition;

enum EFloat 
{
  FNONE = 0, FLEFT, FRIGHT
};
typedef enum EFloat EFloat;

struct RenderBox
{
  int unused[6];
  short m_verticalPosition;
  
  bool m_layouted : 1;
  bool m_unused : 1;
  bool m_minMaxKnown : 1;
  bool m_floating : 1;
  
  bool m_positioned : 1;
  bool m_overhangingContents : 1;
  bool m_relPositioned : 1;
  bool m_paintSpecial : 1;
  
  bool m_isAnonymous : 1;
  bool m_recalcMinMax : 1;
  bool m_isText : 1;
  bool m_inline : 1;
  
  bool m_replaced : 1;
  bool m_mouseInside : 1;
  bool m_hasFirstLine : 1;
  bool m_isSelectionBorder : 1;
  
  bool (*isTableCell) (struct RenderBox *this);
};

typedef struct RenderBox RenderBox;

struct RenderStyle
{
  struct NonInheritedFlags 
    {
      union 
	{
	  struct 
	    {
	      unsigned int _display : 4;
	      unsigned int _bg_repeat : 2;
	      bool _bg_attachment : 1;
	      unsigned int _overflow : 4 ;
	      unsigned int _vertical_align : 4;
	      unsigned int _clear : 2;
	      EPosition _position : 2;
	      EFloat _floating : 2;
	      unsigned int _table_layout : 1;
	      bool _flowAroundFloats :1;

	      unsigned int _styleType : 3;
	      bool _hasHover : 1;
	      bool _hasActive : 1;
	      bool _clipSpecified : 1;
	      unsigned int _unicodeBidi : 2;
	      int _unused : 1;
	    } f;
	  int _niflags;
	};
    } noninherited_flags;
};

typedef struct RenderStyle RenderStyle;

extern void RenderObject_setStyle(RenderBox *this, RenderStyle *_style);
extern void removeFromSpecialObjects(RenderBox *this);



void RenderBox_setStyle(RenderBox *thisin, RenderStyle *_style)
{
  RenderBox *this = thisin;
  bool oldpos, tmp;
  EPosition tmppo;
  
  tmp = this->m_positioned;

  oldpos = tmp;


  RenderObject_setStyle(this, _style);
  
  tmppo = _style->noninherited_flags.f._position;

  switch(tmppo)
    {
    case ABSOLUTE:
    case FIXED:
	{
	  bool ltrue = true;
	  this->m_positioned = ltrue;
	  break;
	}

    default:
	{
	  EFloat tmpf;
	  EPosition tmpp;
	  if (oldpos)
	    {
	      bool ltrue = true;
	      this->m_positioned = ltrue;
	      removeFromSpecialObjects(this);
	    }
	    {
	      bool lfalse = false;
	      this->m_positioned = lfalse;
	    }

	  tmpf = _style->noninherited_flags.f._floating;

	  if(!this->isTableCell (this) && !(tmpf == FNONE)) 
	    {
	      bool ltrue = true;
	      this->m_floating = ltrue;
	    }
	  else 
	    {
	      tmpp = _style->noninherited_flags.f._position;
	      if (tmpp == RELATIVE)
		{
		  bool ltrue = true;
		  this->m_relPositioned = ltrue;
		}
	    }
	}
    }
}




RenderBox g_this;
RenderStyle g__style;

void RenderObject_setStyle(RenderBox *this, RenderStyle *_style)
{
  (void) this;
  (void) _style;
}

void removeFromSpecialObjects(RenderBox *this)
{
  (void) this;
}

bool RenderBox_isTableCell (RenderBox *this)
{
  (void) this;
  return false;
}

int main (void)
{

  g_this.m_relPositioned = false;
  g_this.m_positioned = false;
  g_this.m_floating = false;
  g_this.isTableCell = RenderBox_isTableCell;

  g__style.noninherited_flags.f._position = FIXED;
  g__style.noninherited_flags.f._floating = FNONE;

  RenderBox_setStyle (&g_this, &g__style);
  
  if (g_this.m_positioned != true)
    abort ();
  if (g_this.m_relPositioned != false)
    abort ();
  if (g_this.m_floating != false)
    abort ();

  exit (0);
}
