<?php

/**
 * @file
 * Webform components bundles forms and afferent callbacks.
 *
 * vim:set sw=2 ts=2 et ft=php:
 *
 * @todo prefix all bundle field keys with either bundle machine_name or bundle_
 */

/**
 * Main webform components bundle form.
 *
 * @see webform_components_form()
 */
function webform_component_bundles_components_form($form, &$form_state, $bundle) {
  module_load_include('inc', 'webform', 'includes/webform.components');

  $node = (object) array(
    'nid' => $bundle->bid,
    'webform' => array('components' => $bundle->components),
  );

  $form = webform_components_form($form, $form_state, $node);
  $form['#bundle'] = $bundle;
  $form['nid'] = $form['bid'] = array(
    '#type' => 'value',
    '#value' => $bundle->bid,
  );

  $form['add']['add']['#submit'] = array(
    'webform_component_bundles_components_form_add_submit',
  );
  $form['#submit'] = array('webform_component_bundles_components_form_submit');

  return $form;
}

/**
 * Webform component bundles form submit callback.
 */
function webform_component_bundles_components_form_submit($form, &$form_state) {
  $bundle = webform_component_bundles_load($form['#bundle']->bid);

  // Update all required and weight values.
  $changes = FALSE;
  foreach ($bundle->components as $cid => $component) {
    if ($component['pid'] != $form_state['values']['components'][$cid]['pid'] || $component['weight'] != $form_state['values']['components'][$cid]['weight'] || $component['required'] != $form_state['values']['components'][$cid]['required']) {
      $changes = TRUE;
      $bundle->components[$cid]['weight'] = $form_state['values']['components'][$cid]['weight'];
      $bundle->components[$cid]['required'] = $form_state['values']['components'][$cid]['required'];
      $bundle->components[$cid]['pid'] = $form_state['values']['components'][$cid]['pid'];
    }
  }

  if ($changes) {
    webform_component_bundles_save($bundle);
  }

  drupal_set_message(t('The component positions and required values have been updated.'));
}

/**
 * Webform bundle form component add callback.
 */
function webform_component_bundles_components_form_add_submit($form, &$form_state) {
  $bid = $form_state['values']['bid'];
  $component = $form_state['values']['add'];

  $path = 'admin/content/webform-bundle/' . $bid . '/components/new/' . $component['type'];
  $form_state['redirect'] = array($path,
    array(
      'query' => array(
        'name' => $component['name'],
        'required' => $component['required'],
        'pid' => $component['pid'],
        'weight' => $component['weight'],
      ),
    ),
  );
}

/**
 * Bundle component form.
 *
 * @see webform_component_edit_form()
 */
function webform_component_bundles_component_edit_form($form, $form_state, $bundle, $component, $clone = FALSE) {

  $node = (object) array(
    'nid' => $bundle->bid,
    'webform' => array('components' => $bundle->components),
  );

  module_load_include('inc', 'webform', 'includes/webform.components');
  $form = webform_component_edit_form($form, $form_state, $node, $component, $clone);

  $form['#bundle'] = $bundle;
  $form['bid'] = array(
    '#type' => 'value',
    '#value' => $bundle->bid,
  );

  $form['#validate'] = array(
    'webform_component_bundles_component_edit_form_validate',
  );
  $form['#submit'] = array(
    'webform_component_bundles_component_edit_form_submit',
  );

  if ($clone) {
    drupal_set_title(t('Clone component: @name', array('@name' => $component['name'])), PASS_THROUGH);
  }

  return $form;
}

/**
 * Field name validation for the webform unique key. Must be alphanumeric.
 */
function webform_component_bundles_component_edit_form_validate($form, &$form_state) {
  $bundle = webform_component_bundles_load($form_state['values']['bid']);

  if (!preg_match('/^[a-z0-9_]+$/i', $form_state['values']['form_key'])) {
    form_set_error('form_key', t('The field key %field_key is invalid. Please include only lowercase alphanumeric characters and underscores.', array('%field_key' => $form_state['values']['form_key'])));
  }

  foreach ($bundle->components as $cid => $component) {
    if (($component['cid'] != $form_state['values']['cid'] || $form_state['values']['clone']) && ($component['pid'] == $form_state['values']['pid']) && (strcasecmp($component['form_key'], $form_state['values']['form_key']) == 0)) {
      form_set_error('form_key', t('The field key %field_key is already in use by the field labeled %existing_field. Please use a unique key.', array('%field_key' => $form_state['values']['form_key'], '%existing_field' => $component['name'])));
    }
  }
}

/**
 * Submit handler for webform_component_bundles_component_edit_form().
 */
function webform_component_bundles_component_edit_form_submit($form, &$form_state) {
  // Ensure a webform record exists.
  $bid = $form_state['values']['nid'];
  $bundle = webform_component_bundles_load($bid);

  // Remove empty extra values.
  if (isset($form_state['values']['extra'])) {
    foreach ($form_state['values']['extra'] as $key => $value) {
      if ($value === '' && !isset($form['display'][$key]['#options'][''])) {
        unset($form_state['values']['extra'][$key]);
      }
    }
  }

  // Remove empty attribute values.
  if (isset($form_state['values']['extra']['attributes'])) {
    foreach ($form_state['values']['extra']['attributes'] as $key => $value) {
      if ($value === '') {
        unset($form_state['values']['extra']['attributes'][$key]);
      }
    }
  }

  if ($form_state['values']['clone']) {
    webform_component_bundles_component_clone($bundle, $form_state['values']);
    drupal_set_message(t('Component %name cloned.', array('%name' => $form_state['values']['name'])));
  }
  elseif (!empty($form_state['values']['cid'])) {
    webform_component_bundles_component_update($form_state['values']);

    drupal_set_message(t('Component %name updated.', array('%name' => $form_state['values']['name'])));
  }
  else {
    $cid = webform_component_bundles_component_insert($form_state['values']);
    drupal_set_message(t('New component %name added.', array('%name' => $form_state['values']['name'])));
  }

  $form_state['redirect'] = array('admin/content/webform-bundle/' . $bid . '/components', isset($cid) ? array('query' => array('cid' => $cid)) : array());
}

/**
 * Insert a new component into the database.
 *
 * @param array $component
 *   A full component containing fields from the component form.
 */
function webform_component_bundles_component_insert(&$component) {
  // @todo check the need for this.
  // Allow modules to modify the component before saving.
  // foreach (module_implements('webform_component_presave') as $module) {
  //   $function = $module . '_webform_component_presave';
  //   $function($component);
  // }
  $component['value'] = isset($component['value']) ? $component['value'] : NULL;
  $component['required'] = isset($component['required']) ? $component['required'] : 0;
  $component['extra']['private'] = isset($component['extra']['private']) ? $component['extra']['private'] : 0;

  if (!isset($component['cid'])) {
    if (lock_acquire('webform_bundle_component_insert_' . $component['bid'], 5)) {
      $next_id_query = db_select('webform_bundle_component')->condition('bid', $component['bid']);
      $next_id_query->addExpression('MAX(cid) + 1', 'cid');
      $component['cid'] = $next_id_query->execute()->fetchField();
      if ($component['cid'] == NULL) {
        $component['cid'] = 1;
      }
      lock_release('webform_bundle_component_insert_' . $component['bid']);
    }
    else {
      watchdog('webform', 'A Webform component could not be saved because a timeout occurred while trying to acquire a lock for the bundle. Details: <pre>@component</pre>', array('@component' => print_r($component, TRUE)));
      return FALSE;
    }
  }

  $query = db_insert('webform_bundle_component')
    ->fields(array(
      'bid' => $component['bid'],
      'cid' => $component['cid'],
      'pid' => $component['pid'],
      'form_key' => $component['form_key'],
      'name' => $component['name'],
      'type' => $component['type'],
      'value' => (string) $component['value'],
      'extra' => serialize($component['extra']),
      'required' => (int) $component['required'],
      'weight' => $component['weight'],
    ))
    ->execute();

  return $component['cid'];
}

/**
 * Update an existing component with new values.
 *
 * @param array $component
 *   A full component containing a bid, cid, and all other fields from the
 *   component form. Additional properties are stored in the extra array.
 */
function webform_component_bundles_component_update($component) {
  // Allow modules to modify the component before saving.
  foreach (module_implements('webform_component_presave') as $module) {
    $function = $module . '_webform_component_presave';
    $function($component);
  }

  $component['value'] = isset($component['value']) ? $component['value'] : NULL;
  $component['required'] = isset($component['required']) ? $component['required'] : 0;
  $component['extra']['private'] = isset($component['extra']['private']) ? $component['extra']['private'] : 0;
  db_update('webform_bundle_component')
    ->fields(array(
      'pid' => $component['pid'],
      'form_key' => $component['form_key'],
      'name' => $component['name'],
      'type' => $component['type'],
      'value' => isset($component['value']) ? $component['value'] : '',
      'extra' => serialize($component['extra']),
      'required' => (int) $component['required'],
      'weight' => $component['weight'],
    ))
    ->condition('bid', $component['bid'])
    ->condition('cid', $component['cid'])
    ->execute();
}

/**
 * Recursively insert components into the database.
 *
 * @param object $bundle
 *   The current bundle object containing.
 * @param array $component
 *   A full component containing fields from the component form.
 */
function webform_component_bundles_component_clone(&$bundle, &$component) {
  $original_cid = $component['cid'];
  $component['cid'] = NULL;
  $new_cid = webform_component_bundles_component_insert($component);
  $component['cid'] = $new_cid;

  if (webform_component_feature($component['type'], 'group')) {
    foreach ($bundles->components as $cid => $child_component) {
      if ($child_component['pid'] == $original_cid) {
        $child_component['pid'] = $new_cid;
        webform_component_clone($bundle, $child_component);
      }
    }
  }
  return $new_cid;
}

/**
 * Component bundle delete form.
 */
function webform_component_bundles_component_delete_form($form, $form_state, $bundle, $component) {
  $cid = $component['cid'];

  $form = array(
    'bundle' => array(
      '#type' => 'value',
      '#value' => $bundle,
    ),
    'component' => array(
      '#type' => 'value',
      '#value' => $component,
    ),
  );

  if (webform_component_feature($bundle->components[$cid]['type'], 'group')) {
    $question = t('Delete the %name fieldset?', array('%name' => $bundle->components[$cid]['name']));
    $description = t('This will immediately delete the %name fieldset and all children elements within %name from the %bundle webform component bundle. This cannot be undone.', array('%name' => $bundle->components[$cid]['name'], '%bundle' => $bundle->name));
  }
  else {
    $question = t('Delete the %name component?', array('%name' => $bundle->components[$cid]['name']));
    $description = t('This will immediately delete the %name component from the %bundle webform component bundle. This cannot be undone.', array('%name' => $bundle->components[$cid]['name'], '%bundle' => $bundle->name));
  }

  return confirm_form($form, $question, 'admin/content/webform-bundle/' . $bundle->bid . '/components', $description, t('Delete'));
}

/**
 * Bundle component delete form submit callback.
 */
function webform_component_bundles_component_delete_form_submit($form, &$form_state) {
  // Delete the component.
  $bundle = $form_state['values']['bundle'];
  $component = $form_state['values']['component'];
  webform_component_bundles_component_delete($bundle, $component);
  drupal_set_message(t('Component %name deleted.', array('%name' => $component['name'])));

  $form_state['redirect'] = 'admin/content/webform-bundle/' . $bundle->bid . '/components';
}

/**
 * Deletes webform bundle component.
 */
function webform_component_bundles_component_delete($bundle, $component) {
  // Remove database entries.
  db_delete('webform_bundle_component')
    ->condition('bid', $bundle->bid)
    ->condition('cid', $component['cid'])
    ->execute();

  // Delete all elements under this element.
  $result = db_select('webform_bundle_component', 'c')
    ->fields('c')
    ->condition('bid', $bundle->bid)
    ->condition('pid', $component['cid'])
    ->execute();
  foreach ($result as $row) {
    $child_component = $bundle->components[$row->cid];
    webform_component_bundles_component_delete($bundle, $child_component);
  }
}

/**
 * Validation callback for the bundle add button on webform_components_form().
 */
function webform_component_bundles_form_add_validate($form, &$form_state) {
  // @todo check that the bundle was only added once.
}

/**
 * Submit callback for the bundle add on webform_components_form().
 *
 * @todo insert page break if first/last components have different `page_num`s.
 */
function webform_component_bundles_form_add_submit($form, &$form_state) {
  $bid = $form_state['values']['bundles']['bid'];
  $bundle = webform_component_bundles_load($bid);

  $nid = $form_state['values']['nid'];
  $weight = $form_state['values']['bundles']['weight'];

  // Holds the actual webform inserted components.
  $components = array();

  module_load_include('inc', 'webform', 'components/fieldset');
  $bundle_wrapper_component = _webform_defaults_fieldset();
  $bundle_wrapper_component['type'] = 'fieldset';
  $bundle_wrapper_component['nid'] = $nid;
  $bundle_wrapper_component['name'] = $bundle->name;
  $bundle_wrapper_component['form_key'] = 'bundle_' . $bundle->machine_name;
  $bundle_wrapper_component['weight'] = $weight;

  $bundle_wrapper_cid = webform_component_insert($bundle_wrapper_component);
  $components[$bundle_wrapper_cid] = $bundle_wrapper_component;

  foreach ($bundle->components as $cid => $component) {
    // Remove the cid as this is a new component on the current webform.
    unset($component['cid']);

    $component['nid'] = $nid;
    $component['pid'] = $bundle_wrapper_cid;
    // $component['weight'] += $weight;

    webform_component_insert($component);

    $components[$component['cid']] = $component;
  }

  // Insert page break.
  if ($form_state['values']['bundles']['pagebreak']) {
    module_load_include('inc', 'webform', 'components/pagebreak');
    $break = _webform_defaults_pagebreak();
    $break['type'] = 'pagebreak';
    $break['name'] = 'Page break: ' . $bundle->name;
    $break['nid'] = $nid;
    $break['form_key'] = 'page_' . $bundle->machine_name;
    $break['weight'] = $weight + 1;
    webform_component_insert($break);

    $components[$break['cid']] = $break;
  }

  // Allow other modules to react to the just added bundle to a webform.
  module_invoke_all('webform_component_bundle_add', $bundle, $components);

  // Clear the entity cache if Entity Cache module is installed.
  if (module_exists('entitycache')) {
    cache_clear_all($nid, 'cache_entity_node');
  }
}
