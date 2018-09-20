// This file (my_cntl.cc) was created by Ron Rechenmacher <ron@fnal.gov> on
// Feb  6, 2014. "TERMS AND CONDITIONS" governing this file are in the README
// or COPYING file. If you do not have such a file, one can be obtained by
// contacting Ron or Fermi Lab in Batavia IL, 60510, phone: 630-840-3000.
// $RCSfile: .emacs.gnu,v $
static char* rev=(char*)"$Revision: 1.23 $$Date: 2012/01/23 15:32:40 $";

#include <stdio.h>		// printf
#include <fcntl.h>		// open, O_RDONLY
#include <libgen.h>		// basename
#include <string.h>		// strcmp
#include <stdlib.h>		// strtoul
#include <getopt.h>             // getopt_long

#include "mu2edev.h"		// mu2edev
#include "mu2e_driver/mu2e_mmap_ioctl.h"	// m_ioc_cmd_t

#define USAGE "\
   usage: %s <start|stop|dump>\n\
          %s read <offset>\n\
          %s write <offset> <val>\n\
          %s write_loopback_data [-p<packet_cnt>] [data]...\n\
examples: %s start\n\
", basename(argv[0]), basename(argv[0]), basename(argv[0]), basename(argv[0]), basename(argv[0])




int
main(  int	argc
	   , char	*argv[] )
{
  int                sts=0;
  int                fd;
  char              *cmd;
  m_ioc_cmd_t        ioc_cmd;
  m_ioc_reg_access_t reg_access; 
  mu2edev            dev;
  char devfile[11];
  int dtc = -1;

  int         opt_v=0;
  int         opt;
  int		opt_packets=8;
  while (1)
    {   int option_index = 0;
	  static struct option long_options[] =
        {   /* name,   has_arg, int* flag, val_for_flag */
		  {"mem-to-malloc",1,    0,         'm'},
		  {0,            0,      0,          0},
        };
	  opt = getopt_long (argc, argv, "?hm:Vv",
						 long_options, &option_index);
	  if (opt == -1) break;
	  switch (opt)
        {
        case '?': case 'h':  printf( USAGE );  exit( 0 ); break;
        case 'V': printf("%s\n",rev);return(0);           break;
        case 'v': opt_v++;                                break;
		case 'p': opt_packets=strtoul(optarg,NULL,0);     break;
		case 'd': dtc = strtol(optarg, NULL, 0);          break;
        default:  printf ("?? getopt returned character code 0%o ??\n", opt);
        }
    }
  if (argc - optind < 1)
    {   printf( "Need cmd\n" );
	  printf( USAGE ); exit( 0 );
    }    
  cmd = argv[optind++];
  printf( "cmd=%s\n", cmd );
  printf( "opt_packets=%i\n", opt_packets);

  if (dtc == -1)
  {
	  char* dtcE = getenv("DTCLIB_DTC");
	  if (dtcE != NULL) dtc = strtol(dtcE, NULL, 0);
	  else dtc = 0;
  }

  snprintf(devfile, 11, "/dev/" MU2E_DEV_FILE, dtc);

  fd = open( devfile, O_RDONLY );
  if (fd == -1) { perror("open"); return (1); }

  if      (strcmp(cmd,"start") == 0)
    {
	  sts = ioctl( fd, M_IOC_TEST_START, &ioc_cmd );
    }
  else if (strcmp(cmd,"stop") == 0)
    {
	  sts = ioctl( fd, M_IOC_TEST_STOP, &ioc_cmd );
    }
  else if (strcmp(cmd,"read") == 0)
    {
	  if (argc < 3) { printf(USAGE); return (1); }
	  reg_access.reg_offset = strtoul(argv[2],NULL,0);
	  reg_access.access_type = 0;
	  sts = ioctl( fd, M_IOC_REG_ACCESS, &reg_access );
	  if (sts) { perror("ioctl M_IOC_REG_ACCESS read"); return (1); }
	  printf( "0x%08x\n", reg_access.val );
    }
  else if (strcmp(cmd,"write") == 0)
    {
	  if (argc < 4) { printf(USAGE); return (1); }
	  reg_access.reg_offset  = strtoul(argv[2],NULL,0);
	  reg_access.access_type = 1;
	  reg_access.val         = strtoul(argv[3],NULL,0);
	  sts = ioctl( fd, M_IOC_REG_ACCESS, &reg_access );
	  if (sts) { perror("ioctl M_IOC_REG_ACCESS write"); return (1); }
    }
  else if (strcmp(cmd,"dump") == 0)
    {
	  sts = ioctl( fd, M_IOC_DUMP );
	  if (sts) { perror("ioctl M_IOC_REG_ACCESS write"); return (1); }
    }
  // else if (strcmp(cmd,"write_loopback_data") == 0)
  //   {
  // 	  sts = dev.init();
  // 	  printf("1dev.devfd_=%d\n", dev.get_devfd_() );
  // 	  if (sts) { perror("dev.init"); return (1); }

  // 	  int regs[]={0x9108,0x9168};  // loopback enable
  // 	  for (unsigned ii=0; ii<(sizeof(regs)/sizeof(regs[0])); ++ii)
  // 		{
  // 		  reg_access.reg_offset  = regs[ii];
  // 		  reg_access.access_type = 1;//wr
  // 		  reg_access.val         = 0x1;
  // 		  sts = ioctl( fd, M_IOC_REG_ACCESS, &reg_access );
  // 		  if (sts) { perror("ioctl M_IOC_REG_ACCESS write"); return (1); }
  // 		}
  // 	  reg_access.reg_offset  = 0x9114;  // ring enable register
  // 	  reg_access.access_type = 1;//wr
  // 	  reg_access.val         = 0x101; // Ring 0 - both xmit and recv
  // 	  sts = ioctl( fd, M_IOC_REG_ACCESS, &reg_access );
  // 	  if (sts) { perror("ioctl M_IOC_REG_ACCESS write"); return (1); }

  // 	  reg_access.reg_offset  = 0x2004;  // DMA channel-0 C2S control reg
  // 	  reg_access.access_type = 1;//wr
  // 	  reg_access.val         = 0x100; // bit 8 is DMA Enable (offset 0x2*** is for C2S chan 0)
  // 	  sts = ioctl( fd, M_IOC_REG_ACCESS, &reg_access );
  // 	  if (sts) { perror("ioctl M_IOC_REG_ACCESS write"); return (1); }

  // 	  int chn=0;
  // 	  struct
  // 	  {   DataHeaderPacket hdr;
  // 	    DataPacket       data[8];
  // 	  } data={{{0}}};
  // 	  printf("sizeof(hdr)=%lu\n", sizeof(data.hdr) );
  // 	  printf("sizeof(data)=%lu\n", sizeof(data.hdr) );
  // 	  data.hdr.s.TransferByteCount = 64;
  // 	  if (argc > 2) data.hdr.s.TransferByteCount = strtoul( argv[2],NULL,0 );
  // 	  data.hdr.s.Valid = 1;
  // 	  data.hdr.s.PacketType = 5;  // could be overwritten below
  // 	  data.hdr.s.RingID = 0;
  // 	  data.hdr.s.PacketCount = (data.hdr.s.TransferByteCount-16)/16;// minus header, / packet size
  // 	  //data.hdr.s.ts10 = 0x3210;
  // 	  data.hdr.s.ts32 = 0x7654;
  // 	  data.hdr.s.ts54 = 0xba98;
  // 	  data.hdr.s.data32 = 0xdead;
  // 	  data.hdr.s.data54 = 0xbeef;
  // 	  unsigned pcnt=opt_packets;
  // 	  if (pcnt > (sizeof(data.data)/sizeof(data.data[0])))
  // 		{   pcnt = (sizeof(data.data)/sizeof(data.data[0]));
  // 		}
  // 	  for (unsigned jj=0; jj<pcnt; ++jj)
  // 		{
  // 		  data.data[jj].data10 = (jj<<8)|1;
  // 		  data.data[jj].data32 = (jj<<8)|2;
  // 		  data.data[jj].data54 = (jj<<8)|3;
  // 		  data.data[jj].data76 = (jj<<8)|4;
  // 		  data.data[jj].data98 = (jj<<8)|5;
  // 		  data.data[jj].dataBA = (jj<<8)|6;
  // 		  data.data[jj].dataDC = (jj<<8)|7;
  // 		  data.data[jj].dataFE = (jj<<8)|8;
  // 		}
  // 	  uint16_t *data_pointer = (uint16_t*)&data;
  // 	  unsigned wrds=0;
  // 	  for (int kk=optind; kk<argc; ++kk, ++wrds)
  // 		{   printf("argv[kk]=%s\n",argv[kk] );
  // 		  if (wrds > (sizeof(data.data)/sizeof(uint16_t))) goto out;
  // 		  *data_pointer++ = (uint16_t)strtoul(argv[kk],NULL,0);
  // 		}
  // 	out:

  // 	  printf("2dev.devfd_=%d\n", dev.get_devfd_() );
  // 	  sts = dev.write_loopback_data( chn, &data,data.hdr.s.TransferByteCount );
  //   }
  else
    {
	  printf("unknown cmd %s\n", cmd ); return (1);
    }

  printf( "sts=%d\n", sts );
  return (0);
}   // main
