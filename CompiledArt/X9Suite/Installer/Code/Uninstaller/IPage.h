
#ifndef IPAGE_H_
#define IPAGE_H_

class IPage
{
   public:

                            IPage( void ) { };
                            ~IPage( ) { };

   virtual BOOL             Create      ( HWND parentHwnd ) = 0;
   virtual void             Destroy     ( void ) = 0;

           HWND             m_Hwnd;

};

#endif
