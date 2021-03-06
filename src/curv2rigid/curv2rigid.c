/***************************************************************************/
/* curv2rigid reads curvature and age grids and estimates flexural rigidity*/
/***************************************************************************/

/***************************************************************************
 * Modification history:                                                   *
 *                                                                         *
 ***************************************************************************/

#include"litho.h"

void set_litho_defaults_(Litho *l);
void print_litho_defaults_(Litho *l);
double temp_plt_(Litho *l, double *z, double *age);
double depth_sflr_(Litho *l, double *age);
double ductile_(Litho *l, double *temp, unsigned int *dusw);
double mechthk_(Litho *l, double *age);
double yse_moment_(Litho *l, int *zpts, double *age, double *curv, double *ival1, \
                   double *ival2, double *zmt, double *npstr, unsigned int *wcsw, \
                   unsigned int *znsw, int *rfiter, unsigned int *vsw);
void die (char *s1, char *s2);

char *USAGE = "Usage: curv2rigid age_in.grd curv_in.grd \
               rigid_out.grd bendmo_out.grd yld_out.grd \n \n"
"    age_in.grd     - name of input seafloor age file. \n"
"    curv_in.grd    - name of input curvature file \n"
"    rigid_out.grd  - name of output flexural rigidity file. \n"
"    bendmo_out.grd - name of output bending moment file. \n"
"    yld_out.grd    - name of output depth of yielding file. \n\n";

int
main(int argc, char **argv)
{
     int     i, j, k, m, zid;
     //int     xdim=358, ydim=200000, nz;
     int     xdim=1024, ydim=2048, nz;
     float   *age, *curv, *rigid, *bmo, *zyield, *mt;
     double  *dustra;
     double  ymin, xmin, yinc, xinc, rland, rdum;
     double  ival1, ival2, z, zmt, zp, dz, dsf, zmax;
     double  agept, curvpt, mtpt; 
     double  dustr, temp, npstr, bendmo;
     
     FILE    *master; 
     char    agefilename[128], curvfilename[128], title[128];
     char    rigidfilename[128], bmofilename[128], zyfilename[128];
	 
     /* switches */
     unsigned int verbose = 0;
     unsigned int wcsw; /* do not include water column overburden = 0, include it = 1 */
     unsigned int dusw; /* dorn law = 0, power law = 1 */
     unsigned int znsw = 0;
     unsigned int vsw = 0;     
     
     int riter, totriter;
	 
     Litho lprops;		
     Litho *lptr = &lprops; /* pointer to litho structure */
     
     zmax = 8.e4;
     dz = 4.e2; /* should match dz in yse_moment.c */
     nz=(int)(floor(zmax/dz));
     
     dustra = (double *) malloc(nz * sizeof(double));

     if (argc < 6) die("\n", USAGE);
	 
	/* prepare the output filename */
	 strcpy(agefilename, argv[1]); 
	 strcpy(curvfilename, argv[2]);  
	 strcpy(rigidfilename, argv[3]); 
	 strcpy(bmofilename, argv[4]); 
	 strcpy(zyfilename, argv[5]); 

	 /* allocate the memory for the arrays */
	 age = (float *) malloc(ydim * xdim * sizeof(float));
	 curv = (float *) malloc(ydim * xdim * sizeof(float));
	 rigid = (float *) malloc(ydim * xdim * sizeof(float));
	 bmo = (float *) malloc(ydim * xdim * sizeof(float));
	 zyield = (float *) malloc(ydim * xdim * sizeof(float));

	 readgrd_(age, &xdim, &ydim, &ymin, &xmin, &yinc, &xinc, &rdum, title, agefilename);
	 readgrd_(curv, &xdim, &ydim, &ymin, &xmin, &yinc, &xinc, &rdum, title, curvfilename);

     /* populate litho structure */
     set_litho_defaults_(lptr);
     if(verbose == 1) print_litho_defaults_(lptr);
        
     /* set value for in-plane stress (may be taken from user input later) */
     npstr=0.0;
     riter = 0;
     totriter = 0;   
     
     /* set switch for water column overburden */
     wcsw = 1.0;
     /* move the above to set_litho_defaults */

	 for (j=0; j<ydim; j++){
	     for (k=0; k<xdim; k++) { 
              m = j*xdim + k;
	      agept = (double)age[m];	
	      curvpt = (double)curv[m];	
              mtpt = 8000.*(double)age[m];
              
              if (curvpt < 1.e-10 && curvpt >= 0.0) curvpt = 1.e-10;
              if (curvpt > -1.e-10 && curvpt < 0.0) curvpt = -1.e-10; 
	 		  
		          /* find depth of seafloor from thermal subsidence */
			  dsf=depth_sflr_(lptr,&agept);        
	 
	 		  /* dusw = 0;
	 		  z = 0.5*dz;
	 		  
	 		  for(i=1;i<nz;i++) { 
	 		  	z += dz;
	 			zid = i-1;
	 			temp=temp_plt_(lptr,&z,&agept);
	 			dustra[zid]=ductile_(lptr,&temp,&dusw);
	 			if (dustra[zid] <= 2.e8 && dusw != 1) {
					dusw = 1;
					dustra[zid]=ductile_(lptr,&temp,&dusw);
		 		}  

	 		  } */
	 		  
	 		  zmt = mtpt;		   
              
              if(agept > 5.0) {
                	 ival1=dz;
                  	 ival2=zmt+5.0e3;              
              }
              else {
              		 ival1=dz;
                   	 ival2=zmt+2.0e4;               
              }    
              
              bendmo = yse_moment_(lptr,&nz,&agept,&curvpt,&ival1,&ival2,&zmt,&npstr,&wcsw,&znsw,&riter,&vsw); 
              
              if (znsw == 0) {
              	ival2 = zmax;
              	bendmo = yse_moment_(lptr,&nz,&agept,&curvpt,&ival1,&ival2,&zmt,&npstr,&wcsw,&znsw,&riter,&vsw);
              
              } 
              
              if(znsw == 0) {
              	fprintf(stderr,"Warning: root not found, %d %d age: %lf, mechanical thickness: %lf \n",j,k,agept,zmt);
              	vsw=1;
                bendmo = yse_moment_(lptr,&nz,&agept,&curvpt,&ival1,&ival2,&zmt,&npstr,&wcsw,&znsw,&riter,&vsw);
                vsw=0;
              	
              	return(EXIT_FAILURE);
              }
              
              totriter += riter;
              
              bmo[m] = (float)bendmo;
              rigid[m] = (float)((2.0*bendmo)/(curvpt*(1.0+lptr->pois)));

              /* rigid[m] = (float)(bendmo/curvpt); */
              zyield[m] = (float)lptr->zy;
              
              
              if(rigid[m] < 0.0) {
                  fprintf(stderr,"Warning: negative rigidity \n");
                  vsw=1;
                  bendmo = yse_moment_(lptr,&nz,&agept,&curvpt,&ival1,&ival2,&zmt,&npstr,&wcsw,&znsw,&riter,&vsw);
                  vsw=0;
                  
                  return(EXIT_FAILURE);
              }
        	//fprintf(stderr,"%d %d Total iterations: %d \n",j,riter,totriter);      
		}
		fprintf(stderr,"Finished row %d \n",j+1);      
	 }


	 /* write the grd file */ 
	 rdum = -1.e22; 
	 rland  = -1.e22;
     writegrd_(rigid, &xdim, &ydim, &ymin, &xmin, &yinc, &xinc, &rland, &rdum, title, rigidfilename);
     writegrd_(bmo, &xdim, &ydim, &ymin, &xmin, &yinc, &xinc, &rland, &rdum, title, bmofilename);
     writegrd_(zyield, &xdim, &ydim, &ymin, &xmin, &yinc, &xinc, &rland, &rdum, title, zyfilename);


     return(EXIT_SUCCESS);
}

void die (char *s1, char *s2)
{
        fprintf(stderr," %s %s \n",s1,s2);
        exit(1);
}
