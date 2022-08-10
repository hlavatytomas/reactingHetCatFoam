coating = 83.8e-6
stena = 137e-6/2
WCh = 1e-3*0.5 -10e-5
verPoint2=[-stena-coating,+stena+coating,0.00000e+00]
inPB01 = [
	#[-5.59411e-04,1.02653e-04,0.00000e+00],
	#[-5.49052e-04,1.03595e-04,0.00000e+00],
	#[-5.39634e-04,1.03595e-04,0.00000e+00],
	#[-5.30216e-04,1.03595e-04,0.00000e+00],
	#[-5.20798e-04,1.03595e-04,0.00000e+00],
	#[-5.11381e-04,1.03595e-04,0.00000e+00],
	#[-5.01963e-04,1.03595e-04,0.00000e+00],
	#[-4.92545e-04,1.03595e-04,0.00000e+00],
	#[-4.83128e-04,1.05478e-04,0.00000e+00],
	#[-4.73710e-04,1.05478e-04,0.00000e+00],
	#[-4.64292e-04,1.07362e-04,0.00000e+00],
	#[-4.53933e-04,1.08303e-04,0.00000e+00],
	#[-4.44515e-04,1.08303e-04,0.00000e+00],
	#[-4.35097e-04,1.09245e-04,0.00000e+00],
	#[-4.25680e-04,1.09245e-04,0.00000e+00],
	#[-4.16262e-04,1.10187e-04,0.00000e+00],
	#[-4.06844e-04,1.10187e-04,0.00000e+00],
	#[-3.97427e-04,1.13012e-04,0.00000e+00],
	[-WCh,stena+coating,0.00000e+00],
	[-stena-coating,+stena+coating,0.00000e+00]
]
inPB12 = [
	[-stena-coating,+stena+coating,0.00000e+00],
	[-stena-coating,WCh,0.00000e+00],
]
inTogether = [
	[-WCh,stena+coating,0.00000e+00],
	[-stena-coating,+stena+coating,0.00000e+00],
	[-stena-coating,WCh,0.00000e+00]
]
