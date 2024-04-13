#pragma log /var/log/ethereum/mirror1/check.dcl.log
//#pragma trace 1

  extern char  $address[1024] ;
  extern char  $kind[1024] ;
  extern char  $status[1024] ;
  extern char  $status_next[1024] ;
  extern char  $remark[1024] ;

//********************************************************************
//*                                                                  *
//*                               MAIN                               *

  <unknown>  attr ;
        int  i ;


show("--- Start") ;


        attr.GetAttributes($address, "%") ;

   $status_next<=="oracle___" ;
   $remark     <=="Change status to oracle___" ;

show("--- Done") ;

return ;