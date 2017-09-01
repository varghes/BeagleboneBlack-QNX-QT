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



#include <sys/devi.h>

/* options:
 *
 * Parse command line options.  Create input modules specified on
 * the command line, link them up into event bus lines, and initialize.
 * Initialization consists of allocating the manipulator if necessary,
 * calling its init() callback followed by its parm() callback with
 * any args.  Finally after the module has been linked into an event
 * bus line, the reset() callback is called.
 *
 * Returns  0 on OK
 *         -1 on error
 */

int
options(int argc, char *argv[], uint32_t *reg_flags, unsigned int *opt_flags)
{

        int			opt, i, index, rval;
        input_module_t		*mp = NULL;
        event_bus_line_t	*bp = NULL;

	if (argc < 2) {
		fprintf(stderr, "type 'use %s' for usage\n", argv[0]);
		return (-1);
        }
   
        pFull_process_name = strdup(argv[0]);

	/* parse general devi commmand line options first */

	while ((opt = getopt(argc, argv, "bld:g:vGPrh:")) != -1) {
                switch (opt) {

		  /* disable Ctrl_Alt_Backspace termination of Photon */
                case 'b':
                        *reg_flags |= INP_REG_DISABLE_CASB;
                        break;

			/* open a different device than /dev/photon to inject
			 * events into.  This is legacy stuff from qnx4, we
			 * don't need this anymore since we have the resmgr
			 * interface in nto.
			 */
                case 'd':
                        if((IpTargetDev = malloc(strlen(optarg)+1)) == NULL) {
                                errno_print("malloc");
                                return (0);
                        }
                        strcpy(IpTargetDev, optarg);
                        break;

			/* input group for photon */

                case 'g':
                        IpGroup = atoi(optarg);
                        break;

			/* useful in debugging touchscreens, doesn't require
			 * the prescense of a grafx driver when starting
			 * up a touchscreen driver
			 */
                case 'G':
                        *reg_flags |= INP_REG_NO_GRAFX;
                        break;
                        
			/* don't start the photon interface */
                case 'P':
                        *opt_flags |= OPT_NO_PHOTON;
                        break;
                        
			/* start the resource manager interface */
                case 'r':
                        *opt_flags |= OPT_RESMGR;
                        break;

			/* display a list of available modules */
                        
                case 'l':
                        i = 0;
                        while (mp = ModuleTable[i++]) {

                                printf("%-12s %-12s %c%c%c\n", mp->name, mp->date,
                                       (mp->type & DEVI_MODULE_TYPE_FILTER) ?
                                       'F' : ' ',
                                       (mp->type & DEVI_MODULE_TYPE_PROTO) ?
                                       'P' : ' ',
                                       (mp->type & DEVI_MODULE_TYPE_DEVICE) ?
                                       'D' : ' ');
                        }
                        exit (EXIT_SUCCESS);

			/* debugging verbose level */
                
                case 'v':
                        verbosity++;
                        break;
	        case 'h':
		        pServ_name = strdup(optarg);
		        break;
                }
        }

	/* if user doesn't want the photon i/f, we start the resmgr i/f */
	if (*opt_flags & OPT_NO_PHOTON) 
		*opt_flags |= OPT_RESMGR;

        /* set unbuffered output in verbose mode */

        if (verbosity == 0) {
                close(STDIN_FILENO);
                close(STDOUT_FILENO);
        }
        else {
                setbuf(stdout, NULL);
        }

	/* now process input modules */

        index = optind;
        while (index < argc) {
                if ((mp = module_lookup(argv[index])) == 0) {
		        char * pMsgTxt = "Options: Unable to lookup %s in module table\n";
                        fprintf(stderr,  pMsgTxt, argv[index]);
	                slogf(_SLOG_SETCODE(_SLOGC_INPUT, 0), _SLOG_ERROR, pMsgTxt, argv[index]);
                        return (-1);
                }

                /* 
                 * Initialize the modules by calling its init() and 
                 * parm() callbacks.
                 */

                if (!mp->init) {
		        char * pMsgTxt = "Error: init() callback for module %s does not exist\n";
                        fprintf(stderr, pMsgTxt, mp->name);
	                slogf(_SLOG_SETCODE(_SLOGC_INPUT, 0), _SLOG_ERROR, pMsgTxt, mp->name);
                        return (-1);
                }

                if (mp->init(mp) < 0) {
		        char * pMsgTxt = "Error: init() callback to %s failed\n";
                        fprintf(stderr, pMsgTxt, mp->name);
	                slogf(_SLOG_SETCODE(_SLOGC_INPUT, 0), _SLOG_ERROR, pMsgTxt, mp->name);
                        return (-1);
                }
 
                /* have to reset globals optind,optarg so getopt() will work */
                optind = 1;

                while ((opt = getopt(argc - index, argv + index, 
                                     mp->args)) != -1) {
                        if (!mp->parm) {
		                char * pMsgTxt = "Error: parm() callback for module %s does not exist\n";
                                fprintf(stderr, pMsgTxt, mp->name);
	                        slogf(_SLOG_SETCODE(_SLOGC_INPUT, 0), _SLOG_WARNING, pMsgTxt, mp->name);
                        }
                        mp->parm(mp, opt, optarg);
                }

                /* now add module to bus */

                if ((rval = bus_add(bp, mp)) < 0)
                        return (-1);

		/* bus_add() will return 1 when we need to create a new bus */
                
                if (rval == 1) {
                        if ((bp = bus_create()) == NULL) {
		                char * pMsgTxt = "Error: unable to create bus line\n";
                                fprintf(stderr, pMsgTxt);
	                        slogf(_SLOG_SETCODE(_SLOGC_INPUT, 0), _SLOG_ERROR, pMsgTxt);
                                return (-1);
                        }
                        if (bus_add(bp, mp) < 0)
                                return (-1);
                }
                index += optind;
        }

        /* last one needs to be completed */
        if (bus_complete(bp) < 0)
                return (-1);

        /* now each bus is complete and linked up, call 
         * each modules reset() function */ 

        if (bus_reset() < 0)
                return (-1);

        return (0);
}
 
