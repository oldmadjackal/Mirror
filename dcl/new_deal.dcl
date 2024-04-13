#pragma log /var/log/ethereum/mirror1/new_deal.dcl.log
//#pragma trace 1

  extern char  $path[1024] ;

//********************************************************************
//*                                                                  *
//*                               MAIN                               *

  <unknown>  deal ;
        int  status ;
        int  i ;


show("--- Start") ;

show($path) ;

            deal.CreateDeal() ;

            deal.Kind<=="DealFree" ;
            deal.Data<=="{Deal auto processing}" ;
            deal.UUID<=="" ;

            deal.AddParty("TEST_RAIF", "Bank") ;
            deal.AddParty("TEST_PRIOR", "BankB") ;

            deal.AddStatusMap("New______.1", "Accepted_", "Bank") ;
            deal.AddStatusMap("New______.2", "Rejected_", "Bank") ;
            deal.AddStatusMap("Accepted_.1", "Processed", "Bank") ;
            deal.AddStatusMap("Accepted_.2", "AcceptedB", "BankB") ;

            deal.AddAttribute("Number", "1/1/32") ;
            deal.AddAttribute("Amount", "1000") ;

     status=deal.DeployDeal() ;
  if(status) {
                show(deal.error) ;
                 set_error(601) ;
                  return ;
             }

            deal.AddFile($path, "Order", "Order from customer", "TEST_RAIF", "") ;
     status=deal.SetDealState("Accepted_","Order is confirmed","") ;
  if(status) {
                show(deal.error) ;
                 set_error(601) ;
                  return ;
             }

show("--- Done") ;

return ;
