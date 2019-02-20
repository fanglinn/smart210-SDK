#ifndef __ASM_ARM_ARCH_DRAM_H_
#define __ASM_ARM_ARCH_DRAM_H_

#ifndef __ASSEMBLY__

struct s5pv210_dmc0 {
	unsigned int	concontrol;
	unsigned int	memcontrol;
	unsigned int	memconfig0;
	unsigned int	memconfig1;
	unsigned int	directcmd;
	unsigned int	prechconfig;
	unsigned int	phycontrol0;
	unsigned int	phycontrol1;
	unsigned char	res1[0x08];
	unsigned int	pwrdnconfig;
	unsigned char	res2[0x04];
	unsigned int	timingaref;
	unsigned int	timingrow;
	unsigned int	timingdata;
	unsigned int	timingpower;
	unsigned int	phystatus;
	unsigned int	chip0status;
	unsigned int	chip1status;
	unsigned int	arefstatus;
	unsigned int	mrstatus;
	unsigned int	phytest0;
	unsigned int	phytest1;
};

struct s5pv210_dmc1 {
	unsigned int	concontrol;
	unsigned int	memcontrol;
	unsigned int	memconfig0;
	unsigned int	memconfig1;
	unsigned int	directcmd;
	unsigned int	prechconfig;
	unsigned int	phycontrol0;
	unsigned int	phycontrol1;
	unsigned char	res1[0x08];
	unsigned int	pwrdnconfig;
	unsigned char	res2[0x04];
	unsigned int	timingaref;
	unsigned int	timingrow;
	unsigned int	timingdata;
	unsigned int	timingpower;
	unsigned int	phystatus;
	unsigned int	chip0status;
	unsigned int	chip1status;
	unsigned int	arefstatus;
	unsigned int	mrstatus;
	unsigned int	phytest0;
	unsigned int	phytest1;
};

#endif

#endif
