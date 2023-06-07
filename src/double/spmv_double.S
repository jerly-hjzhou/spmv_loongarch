	.text
	.align 2
	.globl spmv_double
    .type spmv_double, @function
	
spmv_double:
    fsub.d      $f0,    $f0,	$f0
	srli.d		$t1,	$a0,	2
	andi		$t0,	$a0,	0x03

.LOOP_MULX:
	beq			$t1, 	$zero, 	.REST_MULX
	xvld		$xr1,   $a1,    0
	addi.d		$a1, 	$a1, 	32
	xvld        $xr2,   $a2,    0
	addi.d		$a2, 	$a2, 	32
    b           .LOAD_VEC
.RET_LOOP:
	xvfmul.d    	$xr4,   $xr1,   $xr3
	fadd.d      	$f0,    $f0,    $f4
	xvpickve.d  	$xr5,   $xr4,   1
	fadd.d      	$f0,    $f0,    $f5
	xvpickve.d  	$xr5,   $xr4,   2
	fadd.d      	$f0,    $f0,    $f5
	xvpickve.d  	$xr5,   $xr4,   3
	fadd.d      	$f0,    $f0,    $f5
	addi.d      	$t1,     $t1,   -1
	b           	.LOOP_MULX
		
.REST_MULX:
	beq			$t0, 	$zero, 	.END
	fld.d       $f1,    $a1,    0  
	addi.d      $a1,    $a1,    8
	ld.d		$t2,	$a2,	0   
	addi.d		$a2, 	$a2, 	8
	slli.d      $t2,	$t2,    3   
	fldx.d 		$f6,    $a3,	$t2 
	fmul.d      $f7,    $f6,    $f1 
	fadd.d      $f0,    $f0,    $f7
	addi.d		$t0,	$t0,	-1
	b           .REST_MULX

.END:
	jr $ra
	
.LOAD_VEC:
	xvslli.d			$xr2,  	$xr2,   3
	xvpickve2gr.du  	$t2,    $xr2,   0
    fldx.d 		        $f6,    $a3,	$t2 
	xvinsve0.d          $xr3,   $xr6,   0   
	xvpickve2gr.du  	$t2,    $xr2,   1 
    fldx.d 		        $f6,    $a3,	$t2 
	xvinsve0.d          $xr3,   $xr6,   1
	xvpickve2gr.du  	$t2,    $xr2,   2 
   	fldx.d 		        $f6,    $a3,	$t2 
	xvinsve0.d          $xr3,   $xr6,   2
	xvpickve2gr.du  	$t2,    $xr2,   3
    fldx.d 		        $f6,    $a3,	$t2 
	xvinsve0.d          $xr3,   $xr6,   3
	b  					.RET_LOOP

