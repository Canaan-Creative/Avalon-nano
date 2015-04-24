importScripts('sha256.js', 'utils.js');

var thread = {
	enable: true,
	update: false,
	nonce2: 0
};


thread.newJob = function(job, jobId) {
	thread.update = true;
	thread.enable = true;
	thread.job = job;
	thread.jobId = jobId;
	thread.nonce2 = 0;
	thread.update = false;
};

onmessage = function(e) {
	switch (e.data) {
		case "pause":
			thread.enable = false;
			break;
		case "resume":
			thread.enable = true;
			break;
		default:
			thread.newJob(e.data.job, e.data.jobId);
			break;
	}
};

(function loop() {
	var pad = "00000000";

	if (thread.enable && (!thread.update) && thread.job !== undefined) {
		if (thread.nonce2 < 0xffffffff) {
			var nonce2 = thread.nonce2.toString(16);
			var blockheader = get_blockheader(
				thread.job,
				pad.substring(0, pad.length - nonce2.length) + nonce2
			);
			var midstate = get_midstate(blockheader);
			var raw = gw_pool2raw(midstate, blockheader, thread.jobId, 0, 0, thread.nonce2);
			
			var work = [];
			var cnt = Math.ceil(raw.byteLength / 33);
			for (var idx = 1; idx < cnt + 1; idx++)
				work.push(mm_encode(
					P_WORK, 0, idx, cnt,
					raw.slice((idx - 1) * 32, idx * 32)
				));
			postMessage(work);
			(thread.nonce2)++;
		}
	}
	setTimeout(loop, 10);
})();
