/*
 *  Copyright (c) 2018 Nuvoton Technology Corp.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <asm/io.h>

/* Lowlevel init isn't used on nuc980 so just have a dummy here */
__weak void lowlevel_init(void) {}

#if defined(CONFIG_DISPLAY_CPUINFO)
int print_cpuinfo (void)
{
    printf("CPU: NUC980\n");


	return(0);
}
#endif

int arch_cpu_init(void)
{
	// do nothing...

	return 0;
}
