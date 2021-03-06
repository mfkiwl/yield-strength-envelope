#include "litho.h"

/*------------------------------------------------------------*/
/* computes yield strength according to form of Byerlee's law */
/* (values from Mueller and Philllips, 1995)                  */
/*------------------------------------------------------------*/

double temp_plt_(Litho *l, double *z, double *age);
double depth_sflr_(Litho *l, double *age);
double pressure_(Litho *l, double *z, double *dsf, unsigned int *wcsw);
double byerlee_(Litho *l, double *z, double *obp, unsigned int *bysw);
double ductile_(Litho *l, double *temp, unsigned int *dusw);
double elbendstress_(Litho *l, double *zloc, double *curv);

double yse_moment_(Litho *l, int *zpts, double *age, double *curv, double *ival1, \
                   double *ival2, double *zmt, double *npstr, unsigned int *wcsw, \
                   unsigned int *znsw, int *rfiter, unsigned int *vsw)
{
     /* variables for root finding algorithm */
     double wi, itol=2.0e2 ;
     double zmloc, z1loc, z2loc,zrt;
     double ebstrm, ebstr1, ebstr2;
     double efstrm, efstr1, efstr2;
     double npstrm, npstr1, npstr2;
     double z,zn,dsf,temp,obp,zp,bystr,dustr,ystr,bendmo,npstrl;
     double curvl, agel, zmtl;

     int i, j, zid, nz;
      
     int maxiter = 40;
     double dz = 4.e2; /* clunky, probably should be passed from driving program 
    					  or set in litho.h structure */
     
     unsigned int bysw; /* compression = 0, tension = 1 */
     unsigned int dusw; /* dorn law = 0, power law = 1 */
     unsigned int ydsw = 0; /* yielding depth not found = 0, found = 1 */
     unsigned int verbose, wcol;
     
     
     curvl = *curv;
     agel = *age;
     verbose = *vsw;
     wcol = *wcsw;
     zmtl = *zmt;
     nz = *zpts;
     npstrl = *npstr;
     
     double dsa[nz], byc[nz], byt[nz];
	
	 dusw = 0;	
	 npstrm=0.0;
	 npstr1=0.0;
	 npstr2=0.0;
		

     wi=*ival2-*ival1;
	 zn=0.5*(*ival1+*ival2);
		
	 


	 
	 /* this loop constructs the array containing the 
	    ductile stress profile */
	 z=0.5*dz;		 
	 for(j=1;j<nz;j++) { 
	 	z += dz;
	 	zid = j-1;
	 			
	 	temp=temp_plt_(l,&z,&agel);
	 	dsa[zid]=ductile_(l,&temp,&dusw);
	 			
	 	if (dsa[zid] <= 2.e8 && dusw != 1) {
			dusw = 1;
			dsa[zid]=ductile_(l,&temp,&dusw);
		}  
		
		obp=pressure_(l,&z,&dsf,&wcol);
		bysw = 1;
		byt[zid] = byerlee_(l,&z,&obp,&bysw);
		bysw = 0;
		byc[zid] = byerlee_(l,&z,&obp,&bysw);   
		
	 }
	 
	 z=0.5*dz;	
	 /* this loop computes stress profile */
	 for(j=1;j<nz;j++){
		 z += dz;
		 zid = j-1;
		 
		 z1loc=(z-*ival1);
		 z2loc=(z-*ival2);
		 zmloc=(z - zn);

		 
		 // obp=pressure_(l,&z,&dsf,&wcol);
		 
		 

		 
		 ebstr1=elbendstress_(l,&z1loc,&curvl);
		 ebstr2=elbendstress_(l,&z2loc,&curvl);
						 
		 /*
		 temp=temp_plt_(l,&z,&agel); 
		 dustr=ductile_(l,&temp,&dusw); 
		 if (dustr <= 2.e8 && dusw != 1) {
				dusw = 1;
				dustr=ductile_(l,&temp,&dusw);
		 }
		 */
		 
		 /* dustra[didx] = dsa[didx];
		 dustr = dustra[didx]; */
		 
		 dustr = dsa[zid];
		 
		 if(*curv*z1loc >= 0.0){
			  /* bysw = 1;
			  bystr = byerlee_(l,&z,&obp,&bysw); */
			  
			  bystr = byt[zid];
			  
			  ystr = fmin(bystr,dustr);
			  efstr1 = fmin(ystr,ebstr1);
		 }
		 else {
			  /* bysw = 0;
			  bystr = byerlee_(l,&z,&obp,&bysw); */
			  
			  bystr = byc[zid];
			  
			  ystr = fmax(bystr,-dustr);
			  efstr1 = fmax(ystr,ebstr1);
		}
		 
		if(*curv*z2loc >= 0.0){
			  /* bysw = 1;
			  bystr = byerlee_(l,&z,&obp,&bysw); */
			  
			  bystr = byt[zid];
			  
			  ystr = fmin(bystr,dustr);
			  efstr2 = fmin(ystr,ebstr2);
		}
		else {
			  /* bysw = 0;
			  bystr = byerlee_(l,&z,&obp,&bysw); */
			  
			  bystr = byc[zid];
			  
			  ystr = fmax(bystr,-dustr);
			  efstr2 = fmax(ystr,ebstr2);
		}
			 
		npstr1 += efstr1*dz;
		npstr2 += efstr2*dz;               

	}

	npstr1 -= npstrl;
	npstr2 -= npstrl;

                  
	if(npstr1*npstr2 >= 0.0) {
		 if(verbose == 1) {
		 fprintf(stderr,"Nodal plane not in specified interval. \n");
	     fprintf(stderr,"agel: %g, ival1: %g, ival2: %g bendmo: %g, curvpt: %g \n",agel,*ival1,*ival2,bendmo,curvl);
	     }
	     *znsw = 0;
		 return(EXIT_FAILURE);
	}
	
	
	zrt = npstr1 < 0.0 ? (wi=*ival2-*ival1,*ival1) : (wi=*ival1-*ival2,*ival2);            
            
	for(i=1; i<=maxiter; i++) {         
	
		 zn = zrt + (wi *= 0.5);
		 
		 /* switch for function computing ductile deformation induced stresses */
		 // dusw = 0;
		 npstrm=0.0;
		 
		 z = 0.5*dz;
		 /* inner loop computes stress profile */
		 
		 for(j=1;j<nz;j++){
			 z += dz;
			 zid = j-1;
			 
			 zmloc=z-zn;

			 obp=pressure_(l,&z,&dsf,&wcol);
			 
			 
			 ebstrm=elbendstress_(l,&zmloc,&curvl);
			 
			 /* 
			 temp=temp_plt_(l,&z,&agel);
			 dustr=ductile_(l,&temp,&dusw);  				 
			 if (dustr <= 2.e8 && dusw != 1) {
					dusw = 1;
					dustr=ductile_(l,&temp,&dusw);
			 } */
			 
			 dustr = dsa[zid];
			 
			 if(curvl*zmloc >= 0.0){
				  /* bysw = 1;
				  bystr = byerlee_(l,&z,&obp,&bysw); */
				  
				  bystr = byt[zid];
				  
				  ystr = fmin(bystr,dustr);
				  efstrm = fmin(ystr,ebstrm);
			 }
			 else {
			 	  
				  /* bysw = 0;
				  bystr = byerlee_(l,&z,&obp,&bysw); */
				  
				  bystr = byc[zid];
				  
				  
				  ystr = fmax(bystr,-dustr);
				  efstrm = fmax(ystr,ebstrm);
			 }
				 
			npstrm += efstrm*dz;
		}
		
		npstrm -= *npstr;
		
		if(npstrm <= 0.0) zrt = zn;
		
		
		if(npstrm == 0.0) {
			  if(verbose == 1) fprintf(stderr,"Tolerance value for stress difference reached, npstrm: %g \n",npstrm);
			  
			  break;     
		}
		
		if(fabs(wi) < itol) {
			 if(verbose == 1) fprintf(stderr,"Tolerance value for width of interval reached, wi: %g \n",wi);
			 
			 break;
		}		
 
	} 
	
    *znsw = 1;
	*rfiter = i;

	dusw=0; 
	npstrm=0.0;
	bendmo=0.0;
	
	
	l->zn = zn;
	l->zy = 0.;
	
	
	z = 0.5*dz;
	for(j=1;j<nz;j++) {
		 
		 z += dz;
		 zid = j-1;
		   
		 zmloc=(z-zn);

		 
		 obp=pressure_(l,&z,&dsf,&wcol);
		 dustr=ductile_(l,&temp,&dusw);
		 
		 ebstrm=elbendstress_(l,&zmloc,&curvl);
		 
		 
		 /* temp=temp_plt_(l,&z,&agel);
		 if (dustr <= 2.e8 && dusw != 1) {
				dusw = 1;
				dustr=ductile_(l,&temp,&dusw);
		 } */
		 
		 dustr = dsa[zid];
		 
		 if(*curv*zmloc >= 0.0){
			  /* bysw = 1;
			  bystr = byerlee_(l,&z,&obp,&bysw); */
			  
			  bystr = byt[zid];
			  
			  ystr = fmin(bystr,dustr);
			  efstrm = fmin(ystr,ebstrm);
			  if(efstrm != ystr && ydsw == 0) {
			       ydsw = 1;
			       if(z > dz) l->zy = z-dz; 
			       else l->zy = 0.0; 			  
			  }
		 }
		 else {
			  /* bysw = 0;
			  bystr = byerlee_(l,&z,&obp,&bysw); */
			  
			  bystr = byc[zid];
			  
			  ystr = fmax(bystr,-dustr);
			  efstrm = fmax(ystr,ebstrm);
			  if(efstrm != ystr && ydsw == 0) {
			       ydsw = 1;
			       if(z > dz) l->zy = z-dz; 
			       else l->zy = 0.0; 			  
			  }
		 }
		 
		 npstrm += efstrm*dz;
		 bendmo += efstrm*zmloc*dz;
		 
		 
		 // if(verbose == 1) fprintf(stdout,"z %g ystr %g \n",z*1.e-3,ystr*1.e-6); 
    } 
    
    if(l->zy < 0) fprintf(stderr,"agel: %g, zn: %g, bendmo: %g, curvpt: %g \n",agel,zn,bendmo,curvl);
    // if(verbose == 1) fprintf(stderr,"agel: %g, zn: %g, bendmo: %g, curvpt: %g \n",agel,zn,bendmo,curvl);
    return bendmo;
}
