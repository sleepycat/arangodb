/*jslint indent: 2, nomen: true, maxlen: 120, todo: true, white: false, sloppy: false */
/*global require, describe, beforeEach, it, expect, spyOn, createSpy, createSpyObj, afterEach */

////////////////////////////////////////////////////////////////////////////////
/// @brief test for the edge objected handed into pregel algorithm
///
/// @file
///
/// DISCLAIMER
///
/// Copyright 2010-2014 triagens GmbH, Cologne, Germany
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
///
/// Copyright holder is triAGENS GmbH, Cologne, Germany
///
/// @author Michael Hackstein
/// @author Copyright 2014, triAGENS GmbH, Cologne, Germany
////////////////////////////////////////////////////////////////////////////////

var arangodb = require("org/arangodb");
var db = arangodb.db;
var pregel = require("org/arangodb/pregel");
var Edge = pregel.Edge;
var vName = "UnitTestsPregelVertex";
var eOrigName = "UnitTestsPregelEdge";
var eResName = "UnitTestsPregelResult";

describe("Pregel Edge", function () {
  "use strict";

  var orig, res, edgeJSON, edgeId, execNr, testee;

  beforeEach(function () {
    try {
      db._drop(eOrigName);
    } catch (ignore) {}
    try {
      db._drop(eResName);
    } catch (ignore) {}
    try {
      db._drop(vName);
    } catch (ignore) {}
    orig = db._createEdgeCollection(eOrigName);
    res = db._createEdgeCollection(eResName);
    db._create(vName);
    edgeJSON = {
      a: "pregel",
      algorithm: "should",
      run: "very",
      fast: true,
      _from: vName + "/1",
      _to: vName + "/2",
      _key: "myEdge"
    };
    var e = orig.save(edgeJSON._from, edgeJSON._to, edgeJSON);
    var mapping = {
      getResultShard: function () {
        return eResName;
      },
      getResultCollection: function () {
        return eResName;
      },
      getToLocationObject: function () {
        return {
          _id: vName + "/2",
          shard: vName
        };
      }
    };
    edgeId = e._id;
    testee = new Edge(orig.document(edgeId), mapping, eOrigName);
    testee._setResult({a: "old result"});
  });

  afterEach(function () {
    db._drop(eOrigName);
    db._drop(eResName);
    db._drop(vName);
  });

  it("should contain all original attributes", function () {
    Object.keys(edgeJSON).forEach(function (k) {
      expect(testee[k]).toEqual(edgeJSON[k]);
    });
  });

  it("should contain the result stored in result collection", function () {
    expect(testee._getResult().a).toEqual("old result");
  });

  it("should delete the edge", function () {
    testee._delete();
    testee._save();
    expect(res.any().deleted).toBeTruthy();
  });

  it("should inform whether the edge is deleted", function () {
    expect(testee._isDeleted()).toBeFalsy();
    testee._delete();
    expect(testee._isDeleted()).toBeTruthy();
  });

  it("should store a new result", function () {
    var newRes = "new result";
    testee._setResult(newRes);
    testee._save();
    expect(res.any().result).toEqual(newRes);
  });

  it("should store a new sub result", function () {
    var newRes = "new result";
    testee._setResult({a: newRes});
    testee._save();
    expect(res.any().result.a).toEqual(newRes);
  });

  it("should delete removed key", function () {
    var newRes = "new result";
    testee._setResult({b: newRes});
    testee._save();
    var result = res.any().result;
    expect(result.b).toEqual(newRes);
    expect(result.a).toBeUndefined();
  });
});

