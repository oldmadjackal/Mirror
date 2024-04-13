#pragma log /var/log/ethereum/mirror1/DealFree_Accepted_process.dcl.log
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
  <unknown>  files ;
  <unknown>  history ;
  <unknown>  party ;
        int  i ;


show("--- Start " @ $address @ " for status " @ $status) ;


        attr.GetAttributes($address, "%") ;

  for(i=0 ; i<attr.count ; i++)
  {
      show("Attribute " @ attr[i].Name @ "=" @ attr[i].Value) ;
  }
        
        files.GetFiles($address, "") ;

  for(i=0 ; i<files.count ; i++)
  {
      show("File " @ files[i].Kind @ " " @ files[i].Path) ;
      show("  UUID " @ files[i].UUID) ;
      show("  Status/version " @ files[i].Status @ " : " @ files[i].Version) ;
      show("  Remark " @ files[i].Remark) ;
  }

        history.GetHistory($address) ;

  for(i=0 ; i<history.count ; i++)
  {
      show("History " @ history[i].Version @ " " @ history[i].Status) ;
      show("  Remark " @ history[i].Remark) ;
      show("  Actor "  @ history[i].Actor) ;
  }

        party.GetParty($address) ;

  for(i=0 ; i<party.count ; i++)
  {
      show("Party " @ party[i].PartyId @ " " @ party[i].Role) ;
  }
        
   $status_next<=="Processed" ;
   $remark     <=="Change status to Processed" ;

show("--- Done") ;

return ;
