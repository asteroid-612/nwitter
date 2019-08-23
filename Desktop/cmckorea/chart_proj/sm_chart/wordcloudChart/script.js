var testApp = angular.module('testApp', []);
//controller
testApp.controller('testController', function($scope, $http) {

  var temp = location.href.split("?");
  var data = temp[1].split("&");
  var select = data[0];
  // var title = decodeURIComponent(data[1]);
  // console.log(select, title);
  //
  // document.getElementById("title").innerHTML = title;

  var id = [];
  var lat = [];
  var lng = [];
  var name = [];
  var category = [];
  var image = [];

  //파라미터 값 선언
  $scope.paramData = {
    "blogid": "test1",
    "restApiKey": "7b2f8c64c5395060b99bbf650b674e33",
  }
  $scope.paramData = angular.fromJson($scope.paramData);

  $scope.postRequest = function() {

    $scope.push = [];
    $scope.group = [];
    $http.get("http://www.mapplerk3.com/rest/v1/blogs/test1/dftBlogSettingInfo?restApiKey=eeeeb01c378357d1ad1313a5983ffc4e").
    then(function(response) {
      angular.forEach(response, function(list1) {
        angular.forEach(list1.mapDftSettingInfo, function(list2) {
          $scope.push.push(list2);
          angular.forEach($scope.push[14], function(list3) {
            $scope.groupVal = {};
            $scope.groupVal["groupN"] = list3.g_c_seq;
            $scope.groupVal["val"] = list3.category_name;
            $scope.group.push($scope.groupVal);
            //카테고리 이름 같은 것은 같은 배열로 할당
          })
        })
      })
      console.log($scope.group);
      console.log("Successfully get data");
      console.log(response);
    });

    $http({
      method: "POST",
      url: "http://mapplerk2.com/rest/v1/maps/test1",
      data: $scope.paramData,
      dataType: "json",
      headers: {
        'Content-Type': "application/x-www-form-urlencoded"
      }
    }).then(function successCallback(response) {
        console.log("Successfully POST-ed data");
        console.log(response);
        angular.forEach(response, function(list) {
          angular.forEach(list.mapDataList, function(list) {
            //console.log(list);
            id.push(parseInt(list.oid));
            lat.push(parseFloat(list.lat));
            lng.push(parseFloat(list.lng));
            name.push(list.name);
            category.push(list.category);
            image.push(list.s3ThumbObjUrl);
          })
        })
        //console.log(response);
        // 유니크한 데이터에 셀렉트한 값을 저장해놓는다.
        var uniq = [];
        var count = [];
        //select = lat
        //select = $.extend({}, select); // 이거 하면 select에 들어간 스트링이 복사됨. 그게 아닌 것들 줘야하는데..
        //select = window[select];
        //select = eval('('+JSON.stringify(select)+')');

        if (select == "id") {
          select = id;
        } else if (select == "category") {
          select = category;
        } else if (select == "name") {
          select = name;
        } else if (select == "lat") {
          select = lat;
        } else if (select == "lng") {
          select = lng;
        } else if (select == "image") {
          select = image;
        }
        //console.log(select);
        $.each(select, function(i, el) {
          if ($.inArray(el, uniq) === -1) uniq.push(el);
        });

        // 갯수 구하기 위한 함수
        function getCount(arr, val) {
          var ob = {};
          var len = arr.length;
          for (var k = 0; k < len; k++) {
            if (ob.hasOwnProperty(arr[k])) {
              ob[arr[k]]++;
              continue;
            }
            ob[arr[k]] = 1;
          }
          return ob[val];
        }
        for (var i = 0; i < uniq.length; i++) {
          count[i] = getCount(select, uniq[i]);
        }
        // 오브젝트 데이터 넣어주기
        $scope.mapDataObj = {};
        $scope.dataPoint = [];
        for (var i = 0; i < uniq.length; i++) {
          $scope.mapDataObj = {};
          $scope.mapDataObj["text"] = uniq[i],
            $scope.mapDataObj["count"] = count[i],
            $scope.dataPoint.push($scope.mapDataObj);
        }
        //console.log($scope.dataPoint);
        var myConfig = {
          "graphset": [{
            "type": "wordcloud",
            "options": {
              "style": {
                "tooltip": {
                  visible: true,
                  text: '%text: %hits'
                }
              },
              "words": $scope.dataPoint
            }
          }]
        };

        zingchart.render({
          id: 'myChart',
          data: myConfig,
          height: '100%',
          width: '100%'
        });
      },
      function errorCallback(response) {
        console.log("POST-ing of data failed");
      });
  }
});

function changeSelect() {
  var selectVal = document.getElementById("col");
  var changeVal = col.options[selectVal.selectedIndex].value;
  //var temp = location.href.split("?");
  //var data = temp[1].split("&");
  //var title = data[1];
  var url = "./indexT.html?" + changeVal;
  location.href = url;
}
