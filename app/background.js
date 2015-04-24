chrome.app.runtime.onLaunched.addListener(function() {
	chrome.app.window.create('index.html', {
		innerBounds: {
		//bounds: {
			width: 870,
			height: 870,
			minWidth: 870,
			minHeight: 870
		},
	    id: "Avalon Nano"
	});
});
