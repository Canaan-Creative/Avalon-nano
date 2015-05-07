importScripts('sha256.js', 'utils.js');

var thread = {
	enable: true,
	update: false,
	nonce2: 0
};


thread.newJob = function(job, jobId, poolId) {
	thread.update = true;
	thread.enable = true;
	thread.job = job;
	thread.jobId = jobId;
	thread.poolId = poolId;
	thread.nonce2 = 0;
	thread.update = false;
	thread.nonce2_limit = Math.pow(2, thread.job.nonce2_size * 8);
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
			thread.newJob(e.data.job, e.data.jobId, e.data.poolId);
			break;
	}
};

(function loop() {
	if (thread.enable && (!thread.update) && thread.job !== undefined) {
		if (thread.nonce2 < thread.nonce2_limit) {
			var blockheader = get_blockheader(
				thread.job,
				uInt2LeHex(thread.nonce2, thread.job.nonce2_size)
			);

			var midstate = get_midstate(blockheader);
			var raw = gw_pool2raw(
				midstate,
				blockheader,
				thread.poolId,
				thread.jobId,
				0,
				thread.nonce2
			);

			var work = [];
			var cnt = Math.ceil(raw.byteLength / 33);
			for (var idx = 1; idx < cnt + 1; idx++)
				work.push(mm_encode(
					P_WORK, 0, idx, cnt,
					raw.slice((idx - 1) * 32, idx * 32)
				));
			postMessage(work);
			thread.nonce2 = (thread.nonce2 + 1) % thread.nonce2_limit;
		}
	}
	setTimeout(loop, 10);
})();
