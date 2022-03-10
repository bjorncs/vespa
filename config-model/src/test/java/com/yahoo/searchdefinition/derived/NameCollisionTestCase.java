// Copyright Yahoo. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

package com.yahoo.searchdefinition.derived;

import com.yahoo.config.model.deploy.TestProperties;
import com.yahoo.document.DocumentTypeManager;
import com.yahoo.searchdefinition.ApplicationBuilder;

import org.junit.Test;
import static org.junit.Assert.assertThrows;

/**
 * Verifies that a struct in a document type is preferred over another document type
 * of the same name.
 *
 * @author bratseth
 */
public class NameCollisionTestCase extends AbstractExportingTestCase {

    @Test
    public void testNameCollision() throws Exception {
        var ex = assertThrows(RuntimeException.class, () -> {
                assertCorrectDeriving("namecollision", "collisionstruct",
                                      new TestProperties().setExperimentalSdParsing(false),
                                      new TestableDeployLogger());
                var docman = DocumentTypeManager.fromFile("temp/namecollision/documentmanager.cfg");
            });
        ex.printStackTrace();
        System.err.println("MSG 1: "+ex.getClass()+" -> "+ex.getMessage());
        var ey = assertThrows(RuntimeException.class, () -> {
                assertCorrectDeriving("namecollision", "collisionstruct",
                                      new TestProperties().setExperimentalSdParsing(true),
                                      new TestableDeployLogger());
                var docman = DocumentTypeManager.fromFile("temp/namecollision/documentmanager.cfg");
            });
        ey.printStackTrace();
        System.err.println("MSG 2: "+ey.getClass()+" -> "+ey.getMessage());
    }

}
