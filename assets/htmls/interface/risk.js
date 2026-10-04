// Оформленные выпадающие списки поверх настоящих <select>.
// Сам <select> остаётся в документе (скрыт за пределами экрана): его читают
// скрипты страницы и программа, а onclick/onchange вызываются как раньше.

var rsWidgets = [];

function rsInit() {
	var found = document.getElementsByTagName("select");
	var list = [];
	for (var i = 0; i < found.length; i++) list.push(found[i]);
	for (var j = 0; j < list.length; j++) rsWrap(list[j]);
	rsSync();
	setInterval(rsSync, 150);
	document.attachEvent("onmousedown", rsOutside);
}

function rsWrap(sel) {
	var box = document.createElement("span");
	box.className = "rsel";
	var text = document.createElement("span");
	text.className = "rsel-text";
	var arrow = document.createElement("span");
	arrow.className = "rsel-arrow";
	var pop = document.createElement("div");
	pop.className = "rsel-list";
	box.appendChild(text);
	box.appendChild(arrow);
	box.appendChild(pop);
	sel.parentNode.insertBefore(box, sel);
	sel.className = (sel.className ? sel.className + " " : "") + "rsel-native";

	var w = { sel: sel, box: box, text: text, pop: pop, open: false };
	rsWidgets.push(w);
	box.onclick = function () {
		if (w.open) {
			rsClose(w);
			return;
		}
		if (sel.onclick && sel.onclick.call(sel) === false) return;
		rsOpen(w);
	};
}

function rsOpen(w) {
	rsCloseAll();
	var sel = w.sel;
	w.pop.innerHTML = "";
	for (var i = 0; i < sel.options.length; i++) {
		if (!sel.options[i].text) continue;
		var item = document.createElement("div");
		item.className = i == sel.selectedIndex ? "rsel-item rsel-item-sel" : "rsel-item";
		item.innerHTML = rsEscape(sel.options[i].text);
		item.rsIndex = i;
		item.onclick = function (e) {
			var ev = e || window.event;
			ev.cancelBubble = true;
			rsPick(w, this.rsIndex);
		};
		w.pop.appendChild(item);
	}
	w.pop.style.display = "block";
	w.box.style.zIndex = 1000;
	w.box.className = "rsel rsel-open";
	w.open = true;
}

function rsPick(w, index) {
	var changed = w.sel.selectedIndex != index;
	w.sel.selectedIndex = index;
	rsClose(w);
	rsSync();
	if (changed) w.sel.fireEvent("onchange");
}

function rsClose(w) {
	w.pop.style.display = "none";
	w.box.style.zIndex = "";
	w.box.className = "rsel";
	w.open = false;
}

function rsCloseAll() {
	for (var i = 0; i < rsWidgets.length; i++) {
		if (rsWidgets[i].open) rsClose(rsWidgets[i]);
	}
}

function rsOutside() {
	var node = window.event ? window.event.srcElement : null;
	while (node) {
		if (node.className && (" " + node.className + " ").indexOf(" rsel ") >= 0) return;
		if (node.className && (" " + node.className + " ").indexOf(" rsel-open ") >= 0) return;
		node = node.parentNode;
	}
	rsCloseAll();
}

function rsSync() {
	for (var i = 0; i < rsWidgets.length; i++) {
		var w = rsWidgets[i];
		var sel = w.sel;
		var label = sel.selectedIndex >= 0 ? sel.options[sel.selectedIndex].text : "";
		var html = label ? rsEscape(label) : "&nbsp;";
		if (w.text.innerHTML != html) w.text.innerHTML = html;
		var hidden = sel.style.display == "none";
		var display = hidden ? "none" : "";
		if (w.box.style.display != display) w.box.style.display = display;
		if (hidden && w.open) rsClose(w);
	}
}

function rsEscape(s) {
	return s.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;");
}

window.attachEvent("onload", rsInit);
