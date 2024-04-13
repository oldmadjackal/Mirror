#pragma log /var/log/ethereum/mirror1/DealFree_New_process.dcl.log
//#pragma trace 1

  extern char  $address[1024] ;
  extern char  $kind[1024] ;
  extern char  $status[1024] ;
  extern char  $status_next[1024] ;
  extern char  $remark[1024] ;

//********************************************************************
//*                                                                  *
//*                               MAIN                               *

  <unknown>  deal ;
  <unknown>  attr ;
        int  i ;


show("--- Start") ;


        deal.GetDeal($address) ;

    if(deal.count==0) {
                           show("Unknown smart-contract addess: " @ $address) ;
                             return(0) ;
                      }

show("  Kind:" @ deal.Kind) ;
show("  UUID:" @ deal.UUID) ;
show("  Data:" @ deal.Data) ;

        attr.GetAttributes($address, "%") ;

  for(i=0 ; i<attr.count ; i++) {
show("  Attr:" @ attr[i].Name @ "=" @ attr[i].Value) ;
                                }

   $status_next<=="Accepted_" ;
   $remark     <=="Change status to Accepted" ;

show("--- Done") ;

return ;
