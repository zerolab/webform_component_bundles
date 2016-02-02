<?php

/**
 * @file
 * Administrative pages and forms callbacks.
 *
 * vim:set sw=2 ts=2 et ft=php:
 */

/**
 * Menu callback for admin/content/webform-bundle.
 *
 * Displays all webform component bundles.
 */
function webform_component_bundles_admin_content() {
  $query = db_select('webform_bundle')->fields('webform_bundle')
    ->extend('PagerDefault')->limit(20);
  $bundles = $query->execute()->fetchAll();

  $header = array(
    t('Title'),
    t('Description'),
    array('data' => t('Operations'), 'colspan' => '4'),
  );
  $rows = array();

  if (empty($bundles)) {
    $rows[] = array(
      array(
        'data' => t('There are currently no webform component bundles on your site. Please <a href="!url">create</a> one.', array('!url' => url('admin/content/webform-bundle/add'))),
        'colspan' => 5,
      ),
    );
  }
  else {
    foreach ($bundles as $bundle) {
      $rows[] = array(
        l($bundle->name, "admin/content/webform-bundle/{$bundle->bid}/edit"),
        check_plain($bundle->description),
        user_access('edit webform bundle') ? l(t('Edit'), "admin/content/webform-bundle/{$bundle->bid}/edit") : '',
        user_access('edit webform bundle') ? l(t('Manage components'), "admin/content/webform-bundle/{$bundle->bid}/components") : '',
        user_access('edit webform bundle') ? l(t('Clone'), "admin/content/webform-bundle/{$bundle->bid}/clone") : '',
        user_access('delete webform bundle') ? l(t('Delete'), "admin/content/webform-bundle/{$bundle->bid}/delete") : '',
      );
    }
  }

  $output = theme('table', array('header' => $header, 'rows' => $rows));
  $output .= theme('pager', array('tags' => array()));

  return $output;
}

/**
 * Menu callback.
 */
function webform_component_bundles_add_form($form, &$form_state, $bundle = FALSE) {
  $form = array();

  $form['bid'] = array(
    '#type' => 'value',
    '#value' => $bundle ? $bundle->bid : NULL,
  );

  $form['name'] = array(
    '#type' => 'textfield',
    '#title' => t('Name'),
    '#description' => t('Human-readable bundle name'),
    '#required' => TRUE,
    '#default_value' => $bundle ? $bundle->name : '',
  );

  $form['machine_name'] = array(
    '#type' => 'machine_name',
    '#description' => t('A unique machine-readable name for this bundle . It must only contain lowercase letters, numbers and underscores (_).'),
    '#disabled' => 0,
    '#required' => TRUE,
    '#machine_name' => array(
      'exists' => 'webform_component_bundles_bundle_exists',
      'source' => array('name'),
      'error' => t('Please only use lowercase letters, numbers and underscores (_).'),
    ),
    '#default_value' => $bundle ? $bundle->machine_name : '',
  );

  $form['description'] = array(
    '#type' => 'textarea',
    '#title' => t('Description'),
    '#description' => t('Describe this webform component bundle.'),
    '#default_value' => $bundle ? $bundle->description : '',
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Create'),
  );

  return $form;
}

/**
 * Submit callback for webform_component_bundles_add_form().
 */
function webform_component_bundles_add_form_submit($form, &$form_state) {
  $bundle = array(
    'bid' => $form_state['values']['bid'],
    'name' => $form_state['values']['name'],
    'machine_name' => $form_state['values']['machine_name'],
    'description' => $form_state['values']['description'],
  );

  $primary_keys = array();
  if (!empty($bundle['bid'])) {
    $primary_keys = 'bid';
  }

  drupal_write_record('webform_bundle', $bundle, $primary_keys);

  $bid = $bundle['bid'];
  // The form is re-used for edit/clone. Save the bundle id.
  if (!$form_state['values']['bid']) {
    $form_state['values']['bid'] = $bid;
  }

  $form_state['redirect'] = "admin/content/webform-bundle/{$bid}/components";
}

/**
 * Bundle edit form.
 */
function webform_component_bundles_edit_form($form, &$form_state, $bundle) {
  $form = webform_component_bundles_add_form($form, $form_state, $bundle);
  $form['#submit'] = array('webform_component_bundles_add_form_submit');
  $form['submit']['#value'] = t('Update');

  return $form;
}

/**
 * Bundle deletion form.
 */
function webform_component_bundles_delete_form($form, &$form_state, $bundle) {
  $form = array(
    'bundle' => array(
      '#type' => 'value',
      '#value' => $bundle,
    ),
  );

  $question = t('Delete the %name bundle?', array('%name' => $bundle->name));
  $description = t('This will immediately delete the %name bundle. Webforms using this bundle componentes will not be affected. This cannot be undone.', array('%name' => $bundle->name));

  return confirm_form($form, $question, 'admin/content/webform-bundle/' . $bundle->bid . '/components', $description, t('Delete'));
}

/**
 * Bundle delete form submit callback.
 */
function webform_component_bundles_delete_form_submit($form, &$form_state) {
  $bundle = $form_state['values']['bundle'];
  if (!empty($bundle->bid)) {
    db_delete('webform_bundle')
      ->condition('bid', $bundle->bid)
      ->execute();

    drupal_set_message(t('Bundle %name successfully deleted.', array('%name' => $bundle->name)));
  }

  $form_state['redirect'] = 'admin/content/webform-bundle';
}

/**
 * Bundle clone form.
 */
function webform_component_bundles_clone_form($form, &$form_state, $bundle) {
  // The clone and add form are virtually the same.
  // The difference is that the bundle id is set to NULL and
  // we pass the source bundle.
  $form = webform_component_bundles_add_form($form, $form_state, $bundle);

  $form['bid']['#value'] = NULL;

  $form['components'] = array(
    '#type' => 'hidden',
    '#value' => $bundle->components,
  );

  $form['name']['#default_value'] = t('Clone of ') . $form['name']['#default_value'];
  // The machine_name validation will not trigger if a default value is set.
  $form['machine_name']['#default_value'] = '';

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Clone'),
  );

  $form['#submit'] = array(
    'webform_component_bundles_add_form_submit',
    'webform_component_bundles_clone_form_submit',
  );

  return $form;
}

/**
 * Bundle clone form submit callback.
 */
function webform_component_bundles_clone_form_submit($form, &$form_state) {
  if (!$form_state['values']['bid']) {
    return;
  }

  $bid = $form_state['values']['bid'];
  $components = $form_state['values']['components'];

  module_load_include('inc', 'webform_component_bundles', 'includes/webform_component_bundles.components');
  foreach ($components as $component) {
    $component['bid'] = $bid;
    webform_component_bundles_component_insert($component);
  }
}
