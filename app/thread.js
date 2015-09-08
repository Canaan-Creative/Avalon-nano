importScripts('utils.js', 'driver.js');

var enabled = false;
var loopId, job, jobId, poolId, nonce2;

onmessage = function(e) {
	switch (e.data.info) {
		case "stop":
			close();
			break;
		case "pause":
			enabled = false;
			break;
		case "clean":
			job = undefined;
			break;
		case "newJob":
			enabled = false;
			job = e.data.job;
			jqId = e.data.jqId;
			poolId = job.poolId;
			nonce2 = 0;
			// nonce2Limit = Math.pow(2, e.data.nonce2Size * 8);
			/* fall through */
		case "resume":
			if (enabled)
				break;
			clearTimeout(loopId);
			enabled = true;
			loop();
			break;
	}
};

var loop = function() {
	if (job === undefined)
		return;
	// Pay attention to the difference of jqId and jobId,
	// the latter is provided by the pool and used only when submitting.
	var raw = utils.genWork(job, nonce2, poolId, jqId);

	var work = [];
	var cnt = Math.ceil(raw.byteLength / 33);
	for (var idx = 1; idx < cnt + 1; idx++)
		work.push(Avalon.pkgEncode(
			Avalon.P_WORK, 0, idx, cnt,
			raw.slice((idx - 1) * 32, idx * 32)
		));
	postMessage(work, work);
	nonce2++;

	if (enabled)
		loopId = setTimeout(loop, 5);
};
