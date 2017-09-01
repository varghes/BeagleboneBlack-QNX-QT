/*
 * Copyright 2003, QNX Software Systems Ltd. All Rights Reserved.
 *
 * This source code may contain confidential information of QNX Software
 * Systems Ltd.  (QSSL) and its licensors. Any use, reproduction,
 * modification, disclosure, distribution or transfer of this software,
 * or any software which includes or is based upon any of this code, is
 * prohibited unless expressly authorized by QSSL by written agreement. For
 * more information (including whether this source code file has been
 * published) please email licensing@qnx.com.
 */



/* 
 * absolute.c
 *
 * Filter module for absolute bus lines. 
 *
 */
#include <sys/devi.h>

#define FLAG_INIT		0x1000
#define FLAG_XSWAP		0x0100
#define FLAG_YSWAP		0x0200
#define FLAG_BSWAP		0x0400

#define CALIB_DIR               "/etc/system/trap/calib.%s"

static int abs_init(input_module_t *module);
static int abs_reset(input_module_t *module);
static int abs_devctrl(input_module_t *module, int event, void *ptr);
static int abs_input(input_module_t *module, int num, void *ptr);
static int abs_parm(input_module_t *module,int opt, char *optarg);
static int abs_shutdown(input_module_t *module, int delay);

input_module_t	absolute_filter = {

	NULL,
	NULL,
	NULL,
	0,
	DEVI_CLASS_ABS|DEVI_MODULE_TYPE_FILTER,
	"Abs",
	__DATE__,
	"bcxyf:o:s:",
	NULL,
	abs_init,
	abs_reset,
	abs_input,
	NULL,	
	NULL,	
	abs_parm,
	abs_devctrl,
	abs_shutdown,
};


struct data {

	  int			 	flags;
	  struct devctl_disparea_res	res;
	  int				xyswap;
          long				rxl, rxh, ryl, ryh, rzl, rzh; 
          long				sxl, sxh, syl, syh, szl, szh;
          long				rdx,rdy;
          long				sdx,sdy;
          int				num_coords;
          int				num_pressures;
          int				gain;
          int				calib;	
          int				count;
          char				fn[256];
          int                           translate;
};

static void set_trans_range(struct data *dp);
static int init_transform(struct data *dp);
static void transform(struct data *dp, long *x, long *y);

static int 
abs_init(input_module_t *module)
{
        struct data 		*dp = module->data;
        char 			hostname[64];
        char 			*absf;
  
        if(!module->data)  {	
                if(!(dp = module->data = scalloc(sizeof *dp))) {
                        return (-1);
                }
                dp->calib = 0;
                dp->flags = ABSOLUTE;
                dp->sxl = 0;
                dp->sxh = 639;
                dp->syl = 0;
                dp->syh = 479;
                dp->gain = 1;
                dp->translate = 1; /* always translate */
                
                /* get the calibration filename */
    
                /* env var ABSF overrides default */
                
                if((absf = getenv("ABSF")) != NULL)
                        strcpy(dp->fn, absf);
                
                else {
                        
                        if (gethostname(hostname, sizeof(hostname)) < 0)
		            {
                            char * pMsgTxt = "Error: %s - unable to get hostname (%s)\n";
                            fprintf(stderr, pMsgTxt, module->name, strerror(errno));
                            slogf(_SLOG_SETCODE(_SLOGC_INPUT, 0), _SLOG_ERROR, pMsgTxt, module->name, strerror(errno));
			    return EXIT_FAILURE;
			    }
                        sprintf(dp->fn, CALIB_DIR, hostname);
                }
                
                if (verbosity >= 4) {
                        if (!dp->fn)
                                printf("Unable to determine calibration filename\n");
                        else
                                printf("Calibration filename: %s\n", dp->fn);
                }
                
        }
        
        return (0);
}


static int 
abs_reset(input_module_t *module)
{
        struct data 		*dp = module->data;
        input_module_t		*down = module->down;

        if((dp->flags & FLAG_INIT) == 0) {

                struct devctl_coord_range	range;
                unsigned short			flags;

                dp->flags |= FLAG_INIT;

                /* 
                 * Since z transforms are not supported we simply retrieve the
                 * z range from the layer below.  One day absolutef might 
                 * support absolute 3D coordinate pointing devices; 
                 * hence the distinction
                 */

                (down->devctrl)(down, DEVCTL_GETDEVFLAGS, &flags);
                (down->devctrl)(down, DEVCTL_GETPTRPRESS, &dp->num_pressures);
                (down->devctrl)(down, DEVCTL_GETPTRCOORD, &dp->num_coords);


                if(flags & ABS_PRESS_DATA) {

                        if(dp->num_pressures > 1) {
                                char * pMsgTxt = "Error: %s - unsupported input device - multiple pressures\n";
                                fprintf(stderr, pMsgTxt, module->name);
                                slogf(_SLOG_SETCODE(_SLOGC_INPUT, 0), _SLOG_ERROR, pMsgTxt, module->name);
                                return (-1);
                        }
                        (down->devctrl)(down, DEVCTL_GETPRESSRNG, &range);
                } 

                else if (flags & PTR_Z_DATA) {

                        if(dp->num_coords > 2) {
                                char * pMsgTxt = "Error: %s - unsupported input device\n";
                                fprintf(stderr, pMsgTxt, module->name);
                                slogf(_SLOG_SETCODE(_SLOGC_INPUT, 0), _SLOG_ERROR, pMsgTxt, module->name);
                                return (-1);
                        }
                        /* future: */
                        /*  (in->devctrl)(in, DEVCTL_GETCOORDRNG, &range); */
                }

                dp->szl = range.min;
                dp->szh = range.max;
        }

        dp->calib = init_transform(dp);

        /* signal our input layer so that it knows we are installed */
        (down->devctrl)(down, DEVCTL_CHECKPOINT, NULL);
        
        return (0);
}


static int 
abs_devctrl(input_module_t *module, int event, void *ptr)
{
        struct data 	*dp = module->data;
        input_module_t	*down = module->down;

        switch(event) {
                
        case DEVCTL_RESET: {

                struct devctl_disparea_res	*scrn = ptr;
    
                if(scrn) {			
                        dp->sxl = scrn->xl;
                        dp->sxh = scrn->xh;
                        dp->syl = scrn->yl;
                        dp->syh = scrn->yh;
                }
    
                abs_reset(module);
                
                break;
        }			
                
        case DEVCTL_RESCHG: {

                struct devctl_disparea_res	*scrn = ptr;
    
                dp->sxl = scrn->xl;
                dp->sxh = scrn->xh;
                dp->syl = scrn->yl;
                dp->syh = scrn->yh;
                
                set_trans_range(dp);
                
                break;
        }

        case DEVCTL_ABS_MODE: {

                struct devctl_abs_mode *abs_mode  = ptr;
                
                if (abs_mode->mode == ABS_XLATE_MODE) {
                        dp->translate = 1;
                        if (verbosity)
                                printf("DEVCTL_ABS_MODE: xlate\n");
                }
                else {
                        dp->translate = 0;
                        if (verbosity)
                                printf("DEVCTL_ABS_MODE: raw\n");
                }
                break;
        }

        case DEVCTL_COORDCHG: {

                struct devctl_devcoord	*device = ptr;
    
                dp->xyswap = device->swap;
                dp->rxl = device->xl;
                dp->rxh = device->xh;
                dp->ryl = device->yl;
                dp->ryh = device->yh;
                
                dp->calib = 1;
                set_trans_range(dp);
                
                break;
        }
                
        case DEVCTL_GETDEVFLAGS:

                /* get the flags from the layers below */
                (down->devctrl)(down, DEVCTL_GETDEVFLAGS, ptr);
                /* ... and add our info to it */
                *(unsigned long *)ptr |= ABSOLUTE;
                break;

        case DEVCTL_GETPTRBTNS:

                (down->devctrl)(down, DEVCTL_GETPTRBTNS, ptr);
                break;

        case DEVCTL_GETPTRCOORD:

                /* we've already determined number of coordinates at init */
                memcpy(ptr,&dp->num_coords,sizeof(dp->num_coords));
                break;
                
        case DEVCTL_GETPTRPRESS:

                /* we've already determined number of pressures at init */
                memcpy(ptr,&dp->num_pressures,sizeof(dp->num_pressures));
                break;

        case DEVCTL_GETCOORDRNG:

                if(dp->calib) {

                        struct devctl_coord_range	*range = ptr;

                        /* if we're scaling the coordinate then we 
                           determine range */
                        switch(range->which) {
                                
                        case 0:		/* X range being queried */
                                range->min = dp->sxl;
                                range->max = dp->sxh;
                                break;
                        case 1:		/* Y range being queried */
                                range->min = dp->syl;
                                range->max = dp->syh;
                                break;
                        case 2:		/* Z range being queried */
                                range->min = dp->szl;
                                range->max = dp->szh;
                                break;
                        }
                } else {
			(down->devctrl)(down, DEVCTL_GETCOORDRNG, ptr);
                }
                break;

        case DEVCTL_GETPRESSRNG:

                /* we currently don't scale pressure */
                (down->devctrl)(down, DEVCTL_GETPRESSRNG, ptr);
                break;
        }
        
        return (0);
}


static int 
abs_input(input_module_t *module, int num, void *ptr)
{
        struct data			*dp = module->data;
        struct packet_abs		*abuf = (struct packet_abs *) ptr;
        int				i;
        static int			lx,ly,lz,lb;

        for(i = 0; i < num; i++, abuf++) {
                
                if (dp->calib && dp->translate) {
                        struct packet_abs tmp;
    
                        tmp = *abuf;
                        tmp.flags |= ABSOLUTE;
                        tmp.flags &= ~ABS_UNCALIBRATED;


                        /* transform raw coordinates into our 
                           display coordinate system */
                        transform(dp, (long *)&tmp.x, (long *)&tmp.y);


                        if(tmp.x != lx || tmp.y != ly || tmp.z != lz || 
                           tmp.buttons != lb) {

                                lx = tmp.x;
                                ly = tmp.y;
                                lz = tmp.z;
                                lb = tmp.buttons;
                                
                                if(dp->flags & FLAG_BSWAP) {
                                        
                                        switch(tmp.buttons & 0x00000005) {
                                                
                                        case	1L: 
                                                tmp.buttons &= ~1L;
                                                tmp.buttons |= 4L;
                                                break;
                                                
                                                
                                        case 4L: 
                                                tmp.buttons &= ~4L;
                                                tmp.buttons |= 1L;
                                                break;
                                        }
                                }
      
                                /* and send them on their way */
                                devi_enqueue_packet(module, (char *)&tmp, sizeof(tmp));
                        }
                }
                else {
                        
                        if (verbosity >= 3)
                                printf("Raw mode : ");
                        
                        abuf->flags |= ABSOLUTE;
                        if(abuf->x != lx || abuf->y != ly || abuf->z != lz 
                           || abuf->buttons != lb) {
                                
                                lx = abuf->x;
                                ly = abuf->y;
                                lz = abuf->z;
                                lb = abuf->buttons;
                                
			        if(verbosity >= 3)
                                    printf("(%d, %d)\n", lx, ly);
                                
                                /* and send them on their way */
                                devi_enqueue_packet(module, (char *)abuf, 
                                             sizeof(*abuf));
                        }
                }
        }

	return (0);
}


static int 
abs_parm(input_module_t *module,int opt, char *optarg)
{
        struct data *dp = module->data;
        char		*p;
        
        switch(opt) {
                
        case 'b':
                dp->flags |= FLAG_BSWAP;
                break;
                
        case 'x':
                dp->flags ^= FLAG_XSWAP;
                break;
                
        case 'y':
                dp->flags ^= FLAG_YSWAP;
                break;
                
        case 'f':
                strcpy(dp->fn,optarg);
                break;
                
        case 'o':
                p = optarg;
                while(p && (*p++ != ','));
                *(p-1) = '\0';
                dp->sxl = atoi(optarg);
                dp->syl = atoi(p);
                dp->calib = init_transform(dp);
                break;
                
        case 's':
                p = optarg;
                while(p && (*p++ != ','));
                *(p-1) = '\0';
                dp->sxh = atoi(optarg);
                dp->syh = atoi(p);
                dp->sxl = 0;
                dp->syl = 0;
                dp->calib = init_transform(dp);
                break;
                
        default:
                return (-1);
        }
        
        return (0);
}


static int 
abs_shutdown(input_module_t *module, int delay)
{
        return (0);
}


static int 
init_transform(struct data *dp)
{
        int 		found = 0;
        FILE		*calibfile;
        static char	buffer[80];
        char		*p, *delims = ":", *parse = " \n", resl[15], resh[15];
  
        if (verbosity >= 8)
                printf("Attempting to open calibration file: %s\n", dp->fn);


        if((calibfile = fopen(dp->fn, "r")) == NULL) {
                char * pMsgTxt = "Error: %s - cannot open calibration file for absolute filter module(%s)\n";
                fprintf(stderr, pMsgTxt, strerror(errno), dp -> fn);
                slogf(_SLOG_SETCODE(_SLOGC_INPUT, 0), _SLOG_ERROR, pMsgTxt, strerror(errno), dp -> fn);
                
        } else {
                
                sprintf(resl, "%dx%d", (int) dp->sxl,(int) dp->syl);
                sprintf(resh, "%dx%d", (int) dp->sxh, (int) dp->syh);
                
                while((p = fgets(buffer, 80, calibfile)) != NULL) {
                        p = strtok(p,delims);
                        
                        if(p && !strncmp(p, resl, sizeof(resl))) {
                                p=strtok(NULL, delims);
                                
                                if(p && !strncmp(p, resh, sizeof(resh))) {
                                        
                                        if((p = strtok(NULL, delims)) == NULL)
                                                return(0);
                                        
                                        if((p = strtok(p, parse)) == NULL)
                                                return(0);
                                        
                                        dp->rxl = atoi(p);
                                        
                                        if((p = strtok(NULL, parse)) == NULL)
                                                return(0);
                                        
                                        dp->rxh = atoi(p);
                                        
                                        if((p = strtok(NULL, parse)) == NULL)
                                                return(0);
                                        
                                        dp->ryl = atoi(p);
                                        
                                        if((p = strtok(NULL, parse)) == NULL)
                                                return(0);
                                        
                                        dp->ryh = atoi(p);
                                        
                                        if((p = strtok(NULL, parse)) == NULL)
                                                return(0);
                                        
                                        dp->xyswap = atoi(p);
                                        
                                        found = 1;
                                }
                        }
                }
                fclose(calibfile);
        }
  
        if(found) {
                set_trans_range(dp);
        }
  
        if(verbosity >= 4) {
                printf("%s calib info for [%dx%d]\n", found ? "Found" : "Did not find",
                       (int) dp->sxh, (int) dp->syh);
        }
  
        if (verbosity >= 4) {
                
                printf("Calibration parameters\n");
                printf("sxl : %d sxh : %d\n", (int) dp->sxl, 
                       (int) dp->sxh);
                printf("syl : %d syh : %d\n", (int) dp->syl, 
                       (int) dp->syh);
                printf("rxl : %d rxh : %d\n", (int) dp->rxl, 
                       (int) dp->rxh);
                printf("ryl : %d ryh : %d\n", (int) dp->ryl, 
                       (int) dp->ryh);
        }
        
        return (found);
}


static void 
set_trans_range(struct data *dp)
{
        dp->sdx = dp->sxh - dp->sxl;
        dp->sdy = dp->syh - dp->syl;
        
        dp->rdx = dp->rxh - dp->rxl;
        dp->rdy = dp->ryh - dp->ryl;
}


static void 
transform(struct data *dp,long *x,long *y)
{
        long tx,ty;
        
        if(dp->xyswap) {
                tx = *y; ty = *x;
        } else {
                tx = *x; ty = *y;
        }
        
        /* transform X coordinate */

        if(dp->rdx == 0)
                *x = dp->sxl;
        else {
                if(dp->rdx > 0) {
                        if(tx < dp->rxl)
                                tx = dp->rxl;
                        else if(tx > dp->rxh)
                                tx = dp->rxh;
                } else {
                        if(tx > dp->rxl)
                                tx = dp->rxl;
                        else if (tx < dp->rxh)
                                tx = dp->rxh;
		}
                *x = (int)(((long)dp->sdx * 
                            (tx-dp->rxl)) / dp->rdx) + dp->sxl;
        }

        /* transform Y coordinate */

        if(dp->rdy == 0)
                *y = dp->syl;
        else {
                if(dp->rdy > 0) {
                        if(ty < dp->ryl)
                                ty = dp->ryl;
                        else if(ty > dp->ryh)
                                ty = dp->ryh;
                } else {
                        if(ty > dp->ryl)
                                ty = dp->ryl;
                        else if(ty < dp->ryh)
                                ty = dp->ryh;
                }
                *y = (int)(((long)dp->sdy *  
                            (ty - dp->ryl)) / dp->rdy) + dp->syl;
        }
}


