//load visualization API and the piechart package.
google.load('visualization','1',{'packages':['corechart']});

google.setOnLoadCallback(drawChart);

function drawChart(){
	var jsonData = $.ajax({
		url:"sampleData.json",
		dataType:"json",
		async: false,
	}).responseText;


	var data = new google.visualization.DataTable(jsonData);

	var chart = new google.visualization.PieChart(document.getElementById('chart_div'));
	chart.draw(data, {width: 400, height: 240});
}